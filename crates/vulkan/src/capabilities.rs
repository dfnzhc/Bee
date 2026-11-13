use ash::vk;
use std::collections::HashSet;
use std::ffi::CStr;

pub(crate) fn ray_extensions(api_version: u32) -> Vec<&'static CStr> {
    let mut extensions = vec![
        ash::khr::acceleration_structure::NAME,
        ash::khr::ray_tracing_pipeline::NAME,
        ash::khr::deferred_host_operations::NAME,
    ];
    // 这些依赖在 Vulkan 1.2 中已纳入核心 API。
    if api_version < vk::API_VERSION_1_2 {
        extensions.extend([
            ash::khr::buffer_device_address::NAME,
            ash::ext::descriptor_indexing::NAME,
            ash::khr::spirv_1_4::NAME,
            ash::khr::shader_float_controls::NAME,
        ]);
    }
    extensions
}

#[derive(Default)]
pub(crate) struct RayFeatures {
    pub buffer_device_address: bool,
    pub acceleration_structure: bool,
    pub ray_tracing_pipeline: bool,
    pub shader_group_handle_size: u32,
    pub shader_group_handle_alignment: u32,
    pub shader_group_base_alignment: u32,
    pub max_shader_group_stride: u32,
    pub max_ray_recursion_depth: u32,
    pub max_ray_dispatch_invocation_count: u32,
    pub max_ray_hit_attribute_size: u32,
    pub max_geometry_count: u64,
    pub max_primitive_count: u64,
    pub max_instance_count: u64,
    pub max_per_stage_descriptors: u32,
    pub max_set_descriptors: u32,
    pub min_scratch_alignment: u32,
}

pub(crate) fn missing_ray_capabilities(
    api_version: u32,
    extensions: &HashSet<String>,
    features: &RayFeatures,
) -> Vec<String> {
    let mut missing = Vec::new();
    if api_version < vk::API_VERSION_1_1 {
        missing.push("Vulkan 1.1 is required for ray tracing".into());
    }

    // 分别报告缺失的扩展、特性和属性。
    for extension in ray_extensions(api_version) {
        let name = extension
            .to_str()
            .expect("Vulkan extension names are ASCII");
        if !extensions.contains(name) {
            missing.push(format!("extension {name}"));
        }
    }
    for (available, name) in [
        (features.buffer_device_address, "bufferDeviceAddress"),
        (features.acceleration_structure, "accelerationStructure"),
        (features.ray_tracing_pipeline, "rayTracingPipeline"),
    ] {
        if !available {
            missing.push(format!("feature {name}"));
        }
    }

    // 句柄对齐后，着色器组仍须符合设备的步长限制。
    let alignment = features.shader_group_handle_alignment;
    let aligned_handle_size = if alignment.is_power_of_two() {
        features
            .shader_group_handle_size
            .checked_add(alignment - 1)
            .map(|size| size & !(alignment - 1))
    } else {
        None
    };
    if features.shader_group_handle_size == 0
        || !features.shader_group_base_alignment.is_power_of_two()
        || aligned_handle_size.is_none_or(|size| features.max_shader_group_stride < size)
        || features.max_ray_recursion_depth == 0
        || features.max_ray_dispatch_invocation_count == 0
        || features.max_ray_hit_attribute_size < 8
    {
        missing.push("ray tracing pipeline properties".into());
    }
    if features.max_geometry_count == 0
        || features.max_primitive_count == 0
        || features.max_instance_count == 0
        || features.max_per_stage_descriptors == 0
        || features.max_set_descriptors == 0
        || !features.min_scratch_alignment.is_power_of_two()
    {
        missing.push("acceleration structure properties".into());
    }
    missing
}

#[cfg(test)]
mod tests {
    use super::*;

    fn supported_features() -> RayFeatures {
        RayFeatures {
            buffer_device_address: true,
            acceleration_structure: true,
            ray_tracing_pipeline: true,
            shader_group_handle_size: 32,
            shader_group_handle_alignment: 32,
            shader_group_base_alignment: 64,
            max_shader_group_stride: 4096,
            max_ray_recursion_depth: 1,
            max_ray_dispatch_invocation_count: 1,
            max_ray_hit_attribute_size: 8,
            max_geometry_count: 1,
            max_primitive_count: 1,
            max_instance_count: 1,
            max_per_stage_descriptors: 1,
            max_set_descriptors: 1,
            min_scratch_alignment: 256,
        }
    }

    fn supported_extensions(version: u32) -> HashSet<String> {
        ray_extensions(version)
            .into_iter()
            .map(|extension| extension.to_str().unwrap().to_owned())
            .collect()
    }

    #[test]
    fn reports_extensions_features_and_properties_independently() {
        let mut extensions = supported_extensions(vk::API_VERSION_1_2);
        let mut features = supported_features();
        assert!(missing_ray_capabilities(vk::API_VERSION_1_2, &extensions, &features).is_empty());

        extensions.remove("VK_KHR_ray_tracing_pipeline");
        features.buffer_device_address = false;
        features.max_ray_recursion_depth = 0;
        let missing = missing_ray_capabilities(vk::API_VERSION_1_2, &extensions, &features);
        assert!(missing
            .iter()
            .any(|item| item.contains("VK_KHR_ray_tracing_pipeline")));
        assert!(missing
            .iter()
            .any(|item| item.contains("bufferDeviceAddress")));
        assert!(missing
            .iter()
            .any(|item| item.contains("pipeline properties")));
    }

    #[test]
    fn vulkan_1_1_requires_unpromoted_dependencies() {
        let extensions = supported_extensions(vk::API_VERSION_1_2);
        let missing =
            missing_ray_capabilities(vk::API_VERSION_1_1, &extensions, &supported_features());
        assert!(missing
            .iter()
            .any(|item| item.contains("VK_KHR_buffer_device_address")));
        assert!(missing
            .iter()
            .any(|item| item.contains("VK_EXT_descriptor_indexing")));
        assert!(missing.iter().any(|item| item.contains("VK_KHR_spirv_1_4")));
        assert!(missing
            .iter()
            .any(|item| item.contains("VK_KHR_shader_float_controls")));
    }

    #[test]
    fn rejects_unusable_shader_group_properties() {
        let extensions = supported_extensions(vk::API_VERSION_1_2);
        let mut features = supported_features();
        features.shader_group_handle_size = 33;
        features.max_shader_group_stride = 33;
        assert!(
            missing_ray_capabilities(vk::API_VERSION_1_2, &extensions, &features)
                .iter()
                .any(|item| item.contains("pipeline properties"))
        );
        features.shader_group_handle_size = u32::MAX;
        assert!(
            missing_ray_capabilities(vk::API_VERSION_1_2, &extensions, &features)
                .iter()
                .any(|item| item.contains("pipeline properties"))
        );
    }
}
