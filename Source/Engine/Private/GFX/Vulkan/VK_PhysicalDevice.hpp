/**
 * @File VK_PhysicalDevice.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/12/16
 * @Brief This file is part of Bee.
 */

#pragma once

#include <unordered_set>
#include <GFX/Vulkan/VK_Common.hpp>

namespace bee {

class BEE_API VK_PhysicalDevice final
{
public:
    VK_PhysicalDevice(u32 apiVersion, vk::PhysicalDevice physicalDevice);
    BEE_CLASS_MOVABLE_ONLY(VK_PhysicalDevice);

    // physical device capabilities
    BEE_NODISCARD const vk::PhysicalDeviceFeatures& features() const;
    BEE_NODISCARD const vk::PhysicalDeviceProperties& properties() const;
    BEE_NODISCARD const std::vector<vk::QueueFamilyProperties2>& queueFamilyProperties() const;

    BEE_NODISCARD const vk::PhysicalDeviceMemoryProperties& memoryProperties() const;
    BEE_NODISCARD Result<u32> memoryType(uint32_t bits, vk::MemoryPropertyFlags properties) const;

    BEE_NODISCARD vk::PhysicalDeviceFeatures requestedFeatures() const;
    BEE_NODISCARD void* featureList() const;

    // extensions
    void registerExtension(StringView extName, bool required = true);
    BEE_NODISCARD std::vector<const char*> enabledExtensions();
    BEE_NODISCARD bool isExtensionEnabled(StringView extName) const;

    // queues
    void requestQueues(vk::QueueFlags flags, vk::SurfaceKHR surface = VK_NULL_HANDLE);
    BEE_NODISCARD std::vector<std::pair<u32, u32>> availableQueueFamily() const;
    
    BEE_NODISCARD std::optional<u32> graphicsFamilyIndex() const;
    BEE_NODISCARD std::optional<u32> computeFamilyIndex() const;
    BEE_NODISCARD std::optional<u32> transferFamilyIndex() const;
    BEE_NODISCARD std::optional<u32> sparseFamilyIndex() const;
    BEE_NODISCARD std::optional<u32> presentationFamilyIndex() const;

    BEE_NODISCARD u32 graphicsFamilyCount() const;
    BEE_NODISCARD u32 computeFamilyCount() const;
    BEE_NODISCARD u32 transferFamilyCount() const;
    BEE_NODISCARD u32 sparseFamilyCount() const;
    BEE_NODISCARD u32 presentationFamilyCount() const;

    //
    vk::PhysicalDevice handle() const;
    BEE_NODISCARD operator vk::PhysicalDevice() const;

private:
    void setupFeatures();

private:
    vk::PhysicalDevice _handle = VK_NULL_HANDLE;

    vk::PhysicalDeviceFeatures2 _features{};
    vk::PhysicalDeviceFeatures2 _requestedFeatures{};

    vk::PhysicalDeviceProperties2 _properties{};
    vk::PhysicalDeviceMemoryProperties2 _memoryProperties{};
    std::vector<vk::ExtensionProperties> _deviceExtensions{};
    std::vector<vk::QueueFamilyProperties2> _queueProperties{};

    vk::PhysicalDeviceVulkan11Features _features11{};
    vk::PhysicalDeviceVulkan12Features _features12{};
    vk::PhysicalDeviceVulkan13Features _features13{};

    vk::PhysicalDeviceVulkan11Properties _properties11{};
    vk::PhysicalDeviceVulkan12Properties _properties12{};
    vk::PhysicalDeviceVulkan13Properties _properties13{};

    std::unordered_set<String> _enabledDeviceExtensions{};
    std::unordered_map<StringView, bool> _requestedDeviceExtensions{};

    // Properties
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rayTracingPipelineProperties{};
    VkPhysicalDeviceFragmentDensityMapPropertiesEXT _fragmentDensityMapProperties{};
    VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM _fragmentDensityMapOffsetProperties{};

    VulkanChainList<20> _featureList;
    // Features
    vk::PhysicalDeviceMultiviewFeatures _multiviewFeatures{};
    vk::PhysicalDeviceRayQueryFeaturesKHR _rayQueryFeatures{};
    vk::PhysicalDeviceMeshShaderFeaturesNV _meshShaderFeature{};
    vk::PhysicalDeviceMaintenance4Features _maintenance4Features{};
    vk::PhysicalDeviceDescriptorIndexingFeatures _descIndexFeature{};
    vk::PhysicalDeviceTimelineSemaphoreFeatures _timelineSemaphoreFeature{};
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR _accelStructFeatures{};
    vk::PhysicalDeviceBufferDeviceAddressFeatures _bufferDeviceAddressFeatures{};
    vk::PhysicalDeviceFragmentDensityMapFeaturesEXT _fragmentDensityMapFeature{};
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR _rayTracingPipelineFeatures{};
    vk::PhysicalDeviceFragmentShadingRateFeaturesKHR _fragmentShadingRateFeature{};
    vk::PhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM _fragmentDensityMapOffsetFeature{};

    // Queues
    std::optional<u32> _graphicsFamilyIndex;
    std::optional<u32> _computeFamilyIndex;
    std::optional<u32> _transferFamilyIndex;
    std::optional<u32> _sparseFamilyIndex;
    std::optional<u32> _presentationFamilyIndex;

    u32 _graphicsQueueCount     = 0;
    u32 _computeQueueCount      = 0;
    u32 _transferQueueCount     = 0;
    u32 _sparseQueueCount       = 0;
    u32 _presentationQueueCount = 0;
};

} // namespace bee