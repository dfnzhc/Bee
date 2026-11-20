use crate::capabilities::{missing_ray_capabilities, ray_extensions, RayFeatures};
use crate::{DiagnosticLevel, DiagnosticSender};
use anyhow::{anyhow, Context, Result};
use ash::{vk, Entry, Instance};
use std::collections::HashSet;
use std::ffi::{c_void, CStr};
use std::io::Write;
use std::sync::{Arc, Mutex};

const VALIDATION_LAYER: &CStr = c"VK_LAYER_KHRONOS_validation";

#[cfg(test)]
pub(crate) static VALIDATION_ERRORS: std::sync::atomic::AtomicUsize =
    std::sync::atomic::AtomicUsize::new(0);

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ValidationStatus {
    Disabled,
    Enabled,
    EnabledWithoutDebugUtils,
    Unavailable,
}

impl std::fmt::Display for ValidationStatus {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(match self {
            Self::Disabled => "disabled",
            Self::Enabled => "enabled",
            Self::EnabledWithoutDebugUtils => "enabled; debug callback unavailable",
            Self::Unavailable => "requested but unavailable",
        })
    }
}

#[derive(Debug)]
pub struct DeviceReport {
    pub name: String,
    pub api_version: u32,
    pub graphics_queue: u32,
    pub max_image_dimension_2d: u32,
    pub max_color_attachments: u32,
    pub ray_tracing_missing: Vec<String>,
    pub validation: ValidationStatus,
}

pub struct Backend {
    pub report: DeviceReport,
    pub(crate) device: Arc<DeviceState>,
}

pub(crate) struct InstanceState {
    _entry: Entry,
    pub instance: Instance,
    debug: Option<(ash::ext::debug_utils::Instance, vk::DebugUtilsMessengerEXT)>,
    // 地址固定，直到 messenger、实例和在途回调全部销毁后才释放。
    diagnostics: Option<Box<DiagnosticSender>>,
    api_version: u32,
    validation: ValidationStatus,
}

pub(crate) struct DeviceState {
    pub instance: Arc<InstanceState>,
    pub physical_device: vk::PhysicalDevice,
    pub api_version: u32,
    pub ray_tracing_enabled: bool,
    pub device: ash::Device,
    pub queue: vk::Queue,
    pub queue_family: u32,
    pub timestamp_bits: u32,
    pub timestamp_period: f32,
    pub queue_lock: Mutex<()>,
}

impl Drop for InstanceState {
    fn drop(&mut self) {
        unsafe {
            if let Some((debug, messenger)) = &self.debug {
                debug.destroy_debug_utils_messenger(*messenger, None);
            }
            self.instance.destroy_instance(None);
        }
    }
}

impl Drop for DeviceState {
    fn drop(&mut self) {
        unsafe {
            if let Err(error) = self.device.device_wait_idle() {
                eprintln!("Vulkan device wait failed during shutdown: {error:?}");
                if let Some(sender) = &self.instance.diagnostics {
                    sender.send(
                        DiagnosticLevel::Error,
                        "Vulkan",
                        format!("Device wait failed during shutdown: {error:?}"),
                    );
                }
            }
            self.device.destroy_device(None);
        }
    }
}

fn validation_status(requested: bool, layer: bool, debug_utils: bool) -> ValidationStatus {
    match (requested, layer, debug_utils) {
        (false, _, _) => ValidationStatus::Disabled,
        (true, false, _) => ValidationStatus::Unavailable,
        (true, true, false) => ValidationStatus::EnabledWithoutDebugUtils,
        (true, true, true) => ValidationStatus::Enabled,
    }
}

unsafe extern "system" fn debug_callback(
    severity: vk::DebugUtilsMessageSeverityFlagsEXT,
    _types: vk::DebugUtilsMessageTypeFlagsEXT,
    data: *const vk::DebugUtilsMessengerCallbackDataEXT<'_>,
    user_data: *mut c_void,
) -> vk::Bool32 {
    // FFI 边界不得展开 panic，消息复制和输出失败也不能中断 Vulkan。
    let _ = std::panic::catch_unwind(|| unsafe {
        #[cfg(test)]
        if severity.contains(vk::DebugUtilsMessageSeverityFlagsEXT::ERROR) {
            VALIDATION_ERRORS.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
        }
        let message = if data.is_null() || (*data).p_message.is_null() {
            "<no validation message>".into()
        } else {
            CStr::from_ptr((*data).p_message).to_string_lossy()
        };
        let _ = writeln!(
            std::io::stderr().lock(),
            "Vulkan validation {severity:?}: {message}"
        );
        if let Some(sender) = user_data.cast::<DiagnosticSender>().as_ref() {
            let level = if severity.contains(vk::DebugUtilsMessageSeverityFlagsEXT::ERROR) {
                DiagnosticLevel::Error
            } else if severity.contains(vk::DebugUtilsMessageSeverityFlagsEXT::WARNING) {
                DiagnosticLevel::Warning
            } else {
                DiagnosticLevel::Info
            };
            sender.send(level, "Vulkan validation", message.into_owned());
        }
    });
    vk::FALSE
}

impl InstanceState {
    fn new(request_validation: bool, diagnostics: Option<DiagnosticSender>) -> Result<Arc<Self>> {
        let diagnostics = diagnostics.map(Box::new);
        let entry = unsafe { Entry::load() }.context("could not load the Vulkan loader")?;
        let layers = unsafe { entry.enumerate_instance_layer_properties() }
            .context("could not enumerate Vulkan validation layers")?;
        let extensions = unsafe { entry.enumerate_instance_extension_properties(None) }
            .context("could not enumerate Vulkan instance extensions")?;

        // 验证层或调试扩展缺失时，仍允许继续初始化。
        let has_layer = layers
            .iter()
            .any(|layer| unsafe { CStr::from_ptr(layer.layer_name.as_ptr()) } == VALIDATION_LAYER);
        let has_debug = extensions.iter().any(|extension| {
            (unsafe { CStr::from_ptr(extension.extension_name.as_ptr()) })
                == ash::ext::debug_utils::NAME
        });
        let validation = validation_status(request_validation, has_layer, has_debug);
        if matches!(
            validation,
            ValidationStatus::Unavailable | ValidationStatus::EnabledWithoutDebugUtils
        ) {
            eprintln!("Vulkan validation: {validation}");
            if let Some(sender) = &diagnostics {
                sender.send(
                    DiagnosticLevel::Warning,
                    "Vulkan",
                    format!("Validation: {validation}"),
                );
            }
        }

        let layer_names = if has_layer && request_validation {
            vec![VALIDATION_LAYER.as_ptr()]
        } else {
            Vec::new()
        };
        let extension_names = if validation == ValidationStatus::Enabled {
            vec![ash::ext::debug_utils::NAME.as_ptr()]
        } else {
            Vec::new()
        };
        let api_version = unsafe { entry.try_enumerate_instance_version() }
            .context("could not query the Vulkan loader version")?
            .unwrap_or(vk::API_VERSION_1_0)
            .min(vk::API_VERSION_1_2);
        let app_info = vk::ApplicationInfo::default()
            .application_name(c"Bee")
            .engine_name(c"Bee Vulkan")
            .api_version(api_version);
        let mut debug_info = vk::DebugUtilsMessengerCreateInfoEXT::default()
            .message_severity(
                vk::DebugUtilsMessageSeverityFlagsEXT::WARNING
                    | vk::DebugUtilsMessageSeverityFlagsEXT::ERROR,
            )
            .message_type(
                vk::DebugUtilsMessageTypeFlagsEXT::GENERAL
                    | vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION
                    | vk::DebugUtilsMessageTypeFlagsEXT::PERFORMANCE,
            )
            .pfn_user_callback(Some(debug_callback))
            .user_data(
                diagnostics
                    .as_deref()
                    .map_or(std::ptr::null_mut(), |sender| {
                        std::ptr::from_ref(sender).cast_mut().cast()
                    }),
            );
        let mut create_info = vk::InstanceCreateInfo::default()
            .application_info(&app_info)
            .enabled_layer_names(&layer_names)
            .enabled_extension_names(&extension_names);

        // 同时捕获实例创建期间及创建后的验证消息。
        if validation == ValidationStatus::Enabled {
            create_info = create_info.push_next(&mut debug_info);
        }
        let instance = unsafe { entry.create_instance(&create_info, None) }
            .context("could not create the Vulkan instance")?;
        let debug = if validation == ValidationStatus::Enabled {
            let debug = ash::ext::debug_utils::Instance::new(&entry, &instance);
            match unsafe { debug.create_debug_utils_messenger(&debug_info, None) } {
                Ok(messenger) => Some((debug, messenger)),
                Err(error) => {
                    unsafe { instance.destroy_instance(None) };
                    return Err(anyhow!(
                        "could not create Vulkan debug messenger: {error:?}"
                    ));
                }
            }
        } else {
            None
        };
        Ok(Arc::new(Self {
            _entry: entry,
            instance,
            debug,
            diagnostics,
            api_version,
            validation,
        }))
    }
}

fn graphics_queue(queues: &[vk::QueueFamilyProperties]) -> Option<u32> {
    let usable = |queue: &vk::QueueFamilyProperties| {
        queue.queue_count > 0 && queue.queue_flags.contains(vk::QueueFlags::GRAPHICS)
    };
    queues
        .iter()
        .position(|queue| usable(queue) && queue.queue_flags.contains(vk::QueueFlags::COMPUTE))
        .or_else(|| queues.iter().position(usable))
        .map(|index| index as u32)
}

fn query_ray_features(
    instance: &InstanceState,
    physical_device: vk::PhysicalDevice,
    api_version: u32,
    extensions: &HashSet<String>,
) -> RayFeatures {
    let mut result = RayFeatures::default();
    if api_version < vk::API_VERSION_1_1 {
        return result;
    }

    // 只查询核心版本或扩展支持的特性结构。
    let supports = |name: &CStr| extensions.contains(name.to_str().unwrap());
    let has_bda =
        api_version >= vk::API_VERSION_1_2 || supports(ash::khr::buffer_device_address::NAME);
    let has_acceleration = supports(ash::khr::acceleration_structure::NAME);
    let has_pipeline = supports(ash::khr::ray_tracing_pipeline::NAME);
    let mut bda = vk::PhysicalDeviceBufferDeviceAddressFeatures::default();
    let mut acceleration = vk::PhysicalDeviceAccelerationStructureFeaturesKHR::default();
    let mut pipeline = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR::default();
    let mut features = vk::PhysicalDeviceFeatures2::default();
    if has_bda {
        features = features.push_next(&mut bda);
    }
    if has_acceleration {
        features = features.push_next(&mut acceleration);
    }
    if has_pipeline {
        features = features.push_next(&mut pipeline);
    }
    unsafe {
        instance
            .instance
            .get_physical_device_features2(physical_device, &mut features)
    };
    result.buffer_device_address = bda.buffer_device_address == vk::TRUE;
    result.acceleration_structure = acceleration.acceleration_structure == vk::TRUE;
    result.ray_tracing_pipeline = pipeline.ray_tracing_pipeline == vk::TRUE;

    let mut acceleration_properties =
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR::default();
    let mut pipeline_properties = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR::default();
    let mut properties = vk::PhysicalDeviceProperties2::default();
    if has_acceleration {
        properties = properties.push_next(&mut acceleration_properties);
    }
    if has_pipeline {
        properties = properties.push_next(&mut pipeline_properties);
    }
    unsafe {
        instance
            .instance
            .get_physical_device_properties2(physical_device, &mut properties)
    };
    result.shader_group_handle_size = pipeline_properties.shader_group_handle_size;
    result.shader_group_handle_alignment = pipeline_properties.shader_group_handle_alignment;
    result.shader_group_base_alignment = pipeline_properties.shader_group_base_alignment;
    result.max_shader_group_stride = pipeline_properties.max_shader_group_stride;
    result.max_ray_recursion_depth = pipeline_properties.max_ray_recursion_depth;
    result.max_ray_dispatch_invocation_count =
        pipeline_properties.max_ray_dispatch_invocation_count;
    result.max_ray_hit_attribute_size = pipeline_properties.max_ray_hit_attribute_size;
    result.max_geometry_count = acceleration_properties.max_geometry_count;
    result.max_primitive_count = acceleration_properties.max_primitive_count;
    result.max_instance_count = acceleration_properties.max_instance_count;
    result.max_per_stage_descriptors =
        acceleration_properties.max_per_stage_descriptor_acceleration_structures;
    result.max_set_descriptors = acceleration_properties.max_descriptor_set_acceleration_structures;
    result.min_scratch_alignment =
        acceleration_properties.min_acceleration_structure_scratch_offset_alignment;
    result
}

impl Backend {
    pub fn new(request_validation: bool) -> Result<Self> {
        Self::new_with_hidden_extensions(request_validation, &[], None)
    }

    pub fn new_with_diagnostics(
        request_validation: bool,
        diagnostics: DiagnosticSender,
    ) -> Result<Self> {
        Self::new_with_hidden_extensions(request_validation, &[], Some(diagnostics))
    }

    fn new_with_hidden_extensions(
        request_validation: bool,
        hidden_extensions: &[&str],
        diagnostics: Option<DiagnosticSender>,
    ) -> Result<Self> {
        let instance = InstanceState::new(request_validation, diagnostics)?;
        let devices = unsafe { instance.instance.enumerate_physical_devices() }
            .context("could not enumerate Vulkan devices")?;
        let mut failures = Vec::new();

        // 某个设备没有图形队列或创建失败时，继续检查其他设备。
        for physical_device in devices {
            let properties = unsafe {
                instance
                    .instance
                    .get_physical_device_properties(physical_device)
            };
            let name = unsafe { CStr::from_ptr(properties.device_name.as_ptr()) }
                .to_string_lossy()
                .into_owned();
            let queues = unsafe {
                instance
                    .instance
                    .get_physical_device_queue_family_properties(physical_device)
            };
            let Some(queue_family) = graphics_queue(&queues) else {
                failures.push(format!("{name}: no graphics queue"));
                continue;
            };
            let mut extensions = match unsafe {
                instance
                    .instance
                    .enumerate_device_extension_properties(physical_device)
            } {
                Ok(extensions) => extensions
                    .into_iter()
                    .map(|extension| {
                        unsafe { CStr::from_ptr(extension.extension_name.as_ptr()) }
                            .to_string_lossy()
                            .into_owned()
                    })
                    .collect::<HashSet<_>>(),
                Err(error) => {
                    failures.push(format!("{name}: could not query extensions: {error:?}"));
                    continue;
                }
            };
            for hidden in hidden_extensions {
                extensions.remove(*hidden);
            }
            let api_version = instance.api_version.min(properties.api_version);
            let features = query_ray_features(&instance, physical_device, api_version, &extensions);

            // 缺少光线追踪能力不影响图形设备初始化。
            let mut ray_tracing_missing =
                missing_ray_capabilities(api_version, &extensions, &features);
            if !queues[queue_family as usize]
                .queue_flags
                .contains(vk::QueueFlags::COMPUTE)
            {
                ray_tracing_missing
                    .push("graphics queue does not support compute operations".into());
            }
            let priorities = [1.0];
            let queue_infos = [vk::DeviceQueueCreateInfo::default()
                .queue_family_index(queue_family)
                .queue_priorities(&priorities)];
            let mut bda = vk::PhysicalDeviceBufferDeviceAddressFeatures::default()
                .buffer_device_address(true);
            let mut acceleration = vk::PhysicalDeviceAccelerationStructureFeaturesKHR::default()
                .acceleration_structure(true);
            let mut pipeline = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR::default()
                .ray_tracing_pipeline(true);
            let ray_extensions = ray_extensions(api_version);
            let extension_names = ray_extensions
                .iter()
                .map(|name| name.as_ptr())
                .collect::<Vec<_>>();
            let create_device = |with_ray_tracing: bool,
                                 bda: &mut vk::PhysicalDeviceBufferDeviceAddressFeatures<'_>,
                                 acceleration: &mut vk::PhysicalDeviceAccelerationStructureFeaturesKHR<'_>,
                                 pipeline: &mut vk::PhysicalDeviceRayTracingPipelineFeaturesKHR<'_>| {
                let mut create_info = vk::DeviceCreateInfo::default().queue_create_infos(&queue_infos);
                if with_ray_tracing {
                    create_info = create_info.enabled_extension_names(&extension_names)
                        .push_next(bda).push_next(acceleration).push_next(pipeline);
                }
                unsafe { instance.instance.create_device(physical_device, &create_info, None) }
            };

            // 启用光线追踪失败时，回退为仅支持图形的设备。
            let device = if ray_tracing_missing.is_empty() {
                match create_device(true, &mut bda, &mut acceleration, &mut pipeline) {
                    Ok(device) => Ok(device),
                    Err(error) => {
                        ray_tracing_missing
                            .push(format!("could not enable ray tracing: {error:?}"));
                        create_device(false, &mut bda, &mut acceleration, &mut pipeline)
                    }
                }
            } else {
                create_device(false, &mut bda, &mut acceleration, &mut pipeline)
            };
            let device = match device {
                Ok(device) => device,
                Err(error) => {
                    failures.push(format!(
                        "{name}: could not create graphics device: {error:?}"
                    ));
                    continue;
                }
            };
            let queue = unsafe { device.get_device_queue(queue_family, 0) };
            let ray_tracing_enabled = ray_tracing_missing.is_empty();
            let report = DeviceReport {
                name,
                api_version,
                graphics_queue: queue_family,
                max_image_dimension_2d: properties.limits.max_image_dimension2_d,
                max_color_attachments: properties.limits.max_color_attachments,
                ray_tracing_missing,
                validation: instance.validation,
            };
            return Ok(Self {
                report,
                device: Arc::new(DeviceState {
                    instance,
                    physical_device,
                    api_version,
                    ray_tracing_enabled,
                    device,
                    queue,
                    queue_family,
                    timestamp_bits: queues[queue_family as usize].timestamp_valid_bits,
                    timestamp_period: properties.limits.timestamp_period,
                    queue_lock: Mutex::new(()),
                }),
            });
        }
        let detail = if failures.is_empty() {
            "no Vulkan devices were found".into()
        } else {
            failures.join("; ")
        };
        Err(anyhow!("no usable Vulkan graphics device ({detail})"))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn graphics_queue_does_not_depend_on_ray_tracing() {
        let queues = [
            vk::QueueFamilyProperties {
                queue_flags: vk::QueueFlags::COMPUTE,
                queue_count: 1,
                ..Default::default()
            },
            vk::QueueFamilyProperties {
                queue_flags: vk::QueueFlags::GRAPHICS,
                queue_count: 1,
                ..Default::default()
            },
        ];
        assert_eq!(graphics_queue(&queues), Some(1));
        assert_eq!(
            graphics_queue(&[
                vk::QueueFamilyProperties {
                    queue_flags: vk::QueueFlags::GRAPHICS,
                    queue_count: 1,
                    ..Default::default()
                },
                vk::QueueFamilyProperties {
                    queue_flags: vk::QueueFlags::GRAPHICS | vk::QueueFlags::COMPUTE,
                    queue_count: 1,
                    ..Default::default()
                },
            ]),
            Some(1)
        );
        assert_eq!(graphics_queue(&[]), None);
        assert_eq!(
            graphics_queue(&[vk::QueueFamilyProperties {
                queue_flags: vk::QueueFlags::GRAPHICS,
                ..Default::default()
            }]),
            None
        );
    }

    #[test]
    fn validation_reports_available_and_missing_layers() {
        assert_eq!(
            validation_status(true, true, true),
            ValidationStatus::Enabled
        );
        assert_eq!(
            validation_status(true, false, true),
            ValidationStatus::Unavailable
        );
        assert_eq!(
            validation_status(true, true, false),
            ValidationStatus::EnabledWithoutDebugUtils
        );
        assert_eq!(
            validation_status(false, true, true),
            ValidationStatus::Disabled
        );
    }

    #[test]
    fn validation_callback_forwards_levels_and_tolerates_full_or_closed_receiver() {
        let (sender, receiver) = crate::diagnostic_channel();
        let context = Box::new(sender);
        let user_data = std::ptr::from_ref(context.as_ref()).cast_mut().cast();
        let data = vk::DebugUtilsMessengerCallbackDataEXT::default().message(c"callback test");
        for (severity, level) in [
            (
                vk::DebugUtilsMessageSeverityFlagsEXT::INFO,
                DiagnosticLevel::Info,
            ),
            (
                vk::DebugUtilsMessageSeverityFlagsEXT::WARNING,
                DiagnosticLevel::Warning,
            ),
            (
                vk::DebugUtilsMessageSeverityFlagsEXT::ERROR,
                DiagnosticLevel::Error,
            ),
        ] {
            assert_eq!(
                unsafe {
                    debug_callback(
                        severity,
                        vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION,
                        &data,
                        user_data,
                    )
                },
                vk::FALSE
            );
            let messages = receiver.drain().collect::<Vec<_>>();
            assert_eq!(messages.len(), 1);
            assert_eq!(messages[0].level, level);
            assert_eq!(messages[0].source, "Vulkan validation");
            assert_eq!(messages[0].message, "callback test");
        }
        for _ in 0..crate::DIAGNOSTIC_QUEUE_CAPACITY {
            context.send(DiagnosticLevel::Info, "test", "fill queue");
        }
        unsafe {
            debug_callback(
                vk::DebugUtilsMessageSeverityFlagsEXT::WARNING,
                vk::DebugUtilsMessageTypeFlagsEXT::GENERAL,
                std::ptr::null(),
                user_data,
            );
        }
        assert_eq!(receiver.take_dropped(), 1);
        drop(receiver);
        unsafe {
            debug_callback(
                vk::DebugUtilsMessageSeverityFlagsEXT::WARNING,
                vk::DebugUtilsMessageTypeFlagsEXT::GENERAL,
                &data,
                user_data,
            );
        }
        unsafe {
            debug_callback(
                vk::DebugUtilsMessageSeverityFlagsEXT::WARNING,
                vk::DebugUtilsMessageTypeFlagsEXT::GENERAL,
                std::ptr::null(),
                std::ptr::null_mut(),
            );
        }
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device and validation layer"]
    fn validation_messenger_retains_context_through_shutdown() -> Result<()> {
        let (sender, receiver) = crate::diagnostic_channel();
        let backend = Backend::new_with_diagnostics(true, sender)?;
        assert_eq!(backend.report.validation, ValidationStatus::Enabled);
        let device = backend.device.clone();
        drop(backend);
        let (debug, _) = device.instance.debug.as_ref().unwrap();
        let data = vk::DebugUtilsMessengerCallbackDataEXT::default()
            .message(c"Bee diagnostic callback lifecycle test");
        unsafe {
            debug.submit_debug_utils_message(
                vk::DebugUtilsMessageSeverityFlagsEXT::WARNING,
                vk::DebugUtilsMessageTypeFlagsEXT::GENERAL,
                &data,
            );
        }
        assert!(receiver
            .drain()
            .any(|message| message.level == DiagnosticLevel::Warning
                && message
                    .message
                    .contains("Bee diagnostic callback lifecycle test")));
        // 接收端先关闭，设备及实例销毁期间的回调仍持有有效上下文。
        drop(receiver);
        drop(device);
        Ok(())
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device"]
    fn graphics_device_initializes_without_ray_tracing() -> Result<()> {
        let native = Backend::new(true)?;
        println!("Native Vulkan capabilities: {:?}", native.report);
        let backend =
            Backend::new_with_hidden_extensions(true, &["VK_KHR_ray_tracing_pipeline"], None)?;
        assert_eq!(backend.report.validation, ValidationStatus::Enabled);
        assert!(backend
            .report
            .ray_tracing_missing
            .iter()
            .any(|item| item.contains("VK_KHR_ray_tracing_pipeline")));
        assert!(backend.create_path_tracer().is_err());
        assert!(backend.build_acceleration_scene(&[], 0).is_err());
        assert_ne!(backend.device.queue, vk::Queue::null());
        Ok(())
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device"]
    fn missing_validation_layer_is_reported() {
        if std::env::var_os("BEE_TEST_NO_LAYER").is_some() {
            let (sender, receiver) = crate::diagnostic_channel();
            let backend = Backend::new_with_diagnostics(true, sender).unwrap();
            assert_eq!(backend.report.validation, ValidationStatus::Unavailable);
            assert!(receiver
                .drain()
                .any(|message| message.level == DiagnosticLevel::Warning
                    && message.message.contains("requested but unavailable")));
            return;
        }
        let output = std::process::Command::new(std::env::current_exe().unwrap())
            .args([
                "--exact",
                "device::tests::missing_validation_layer_is_reported",
                "--ignored",
                "--nocapture",
            ])
            .env("BEE_TEST_NO_LAYER", "1")
            .env("VK_LAYER_PATH", "bee-no-such-vulkan-layer-directory")
            .output()
            .unwrap();
        assert!(
            output.status.success(),
            "{}\n{}",
            String::from_utf8_lossy(&output.stdout),
            String::from_utf8_lossy(&output.stderr)
        );
    }
}
