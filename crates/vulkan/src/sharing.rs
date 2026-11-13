use crate::device::Backend;
use anyhow::{Context, Result};
use ash::vk;
use std::ffi::CStr;

#[derive(Debug, Default)]
pub struct ExternalHandleSupport {
    pub importable: bool,
    pub exportable: bool,
}

#[derive(Debug, Default)]
pub struct SharingProbe {
    pub device_luid: Option<[u8; 8]>,
    pub d3d11_texture: ExternalHandleSupport,
    pub opaque_win32_memory: ExternalHandleSupport,
    pub d3d12_fence: ExternalHandleSupport,
    pub opaque_win32_semaphore: ExternalHandleSupport,
    pub keyed_mutex: bool,
}

impl Backend {
    pub fn probe_sharing(&self) -> Result<SharingProbe> {
        if self.report.api_version < vk::API_VERSION_1_1 {
            anyhow::bail!("Vulkan 1.1 is required to query external handles");
        }
        let instance = &self.device.instance.instance;
        let physical = self.device.physical_device;
        let extensions = unsafe { instance.enumerate_device_extension_properties(physical) }
            .context("could not query external handle extensions")?;
        let supports = |name: &CStr| {
            extensions.iter().any(
                |extension| unsafe { CStr::from_ptr(extension.extension_name.as_ptr()) } == name,
            )
        };
        let mut id = vk::PhysicalDeviceIDProperties::default();
        let mut properties = vk::PhysicalDeviceProperties2::default().push_next(&mut id);
        unsafe { instance.get_physical_device_properties2(physical, &mut properties) };
        let mut report = SharingProbe {
            device_luid: (id.device_luid_valid == vk::TRUE).then_some(id.device_luid),
            keyed_mutex: supports(ash::khr::win32_keyed_mutex::NAME),
            ..Default::default()
        };

        // 探测可用作附件、传输和采样的 BGRA 图像共享能力。
        if supports(ash::khr::external_memory_win32::NAME) {
            let image_support = |handle_type| -> Result<ExternalHandleSupport> {
                let mut external_info =
                    vk::PhysicalDeviceExternalImageFormatInfo::default().handle_type(handle_type);
                let info = vk::PhysicalDeviceImageFormatInfo2::default()
                    .format(vk::Format::B8G8R8A8_UNORM)
                    .ty(vk::ImageType::TYPE_2D)
                    .tiling(vk::ImageTiling::OPTIMAL)
                    .usage(
                        vk::ImageUsageFlags::COLOR_ATTACHMENT
                            | vk::ImageUsageFlags::TRANSFER_SRC
                            | vk::ImageUsageFlags::SAMPLED,
                    )
                    .push_next(&mut external_info);
                let mut external = vk::ExternalImageFormatProperties::default();
                let mut properties = vk::ImageFormatProperties2::default().push_next(&mut external);
                match unsafe {
                    instance.get_physical_device_image_format_properties2(
                        physical,
                        &info,
                        &mut properties,
                    )
                } {
                    Ok(()) => {
                        let features = external.external_memory_properties.external_memory_features;
                        Ok(ExternalHandleSupport {
                            importable: features
                                .contains(vk::ExternalMemoryFeatureFlags::IMPORTABLE),
                            exportable: features
                                .contains(vk::ExternalMemoryFeatureFlags::EXPORTABLE),
                        })
                    }
                    Err(vk::Result::ERROR_FORMAT_NOT_SUPPORTED) => Ok(Default::default()),
                    Err(error) => Err(error).context("could not query external image support"),
                }
            };
            report.d3d11_texture = image_support(vk::ExternalMemoryHandleTypeFlags::D3D11_TEXTURE)?;
            report.opaque_win32_memory =
                image_support(vk::ExternalMemoryHandleTypeFlags::OPAQUE_WIN32)?;
        }

        // 单独探测同步句柄，不与图像内存共享能力混同。
        if supports(ash::khr::external_semaphore_win32::NAME) {
            let semaphore_support = |handle_type| {
                let info =
                    vk::PhysicalDeviceExternalSemaphoreInfo::default().handle_type(handle_type);
                let mut properties = vk::ExternalSemaphoreProperties::default();
                unsafe {
                    instance.get_physical_device_external_semaphore_properties(
                        physical,
                        &info,
                        &mut properties,
                    )
                };
                ExternalHandleSupport {
                    importable: properties
                        .external_semaphore_features
                        .contains(vk::ExternalSemaphoreFeatureFlags::IMPORTABLE),
                    exportable: properties
                        .external_semaphore_features
                        .contains(vk::ExternalSemaphoreFeatureFlags::EXPORTABLE),
                }
            };
            report.d3d12_fence =
                semaphore_support(vk::ExternalSemaphoreHandleTypeFlags::D3D12_FENCE);
            report.opaque_win32_semaphore =
                semaphore_support(vk::ExternalSemaphoreHandleTypeFlags::OPAQUE_WIN32);
        }
        Ok(report)
    }
}
