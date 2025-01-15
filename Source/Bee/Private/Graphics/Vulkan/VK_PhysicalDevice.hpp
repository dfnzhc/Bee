/**
 * @File VK_PhysicalDevice.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/12/16
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Graphics/Vulkan/VK_Common.hpp"

namespace bee {
class BEE_API VK_PhysicalDevice final
{
public:
    VK_PhysicalDevice() = default;
    ~VK_PhysicalDevice() = default;
    BEE_CLASS_MOVABLE_ONLY(VK_PhysicalDevice);

    BEE_NODISCARD Error setup(vk::PhysicalDevice physicalDevice);

    // physical device capabilities;
    BEE_NODISCARD const std::vector<vk::QueueFamilyProperties2>& queueFamilyProperties() const;

    BEE_NODISCARD const vk::PhysicalDeviceMemoryProperties& memoryProperties() const;
    BEE_NODISCARD Result<u32> memoryType(uint32_t bits, vk::MemoryPropertyFlags properties) const;

    // extensions
    void registerExtension(StringView extName, bool required = true);
    void finalizeRegister();
    BEE_NODISCARD std::vector<const char*> enabledExtensions() const;
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

    BEE_NODISCARD bool isSupportPresent(vk::SurfaceKHR surface) const;

    //
    vk::PhysicalDevice handle() const;
    BEE_NODISCARD operator vk::PhysicalDevice() const;

    // device capabilities
    struct MultiviewCapabilities
    {
        bool isSupported = false;
        bool geometryShaderSupported = false;
        bool tessellationShaderSupported = false;
        uint32_t maxViewCount = 0;
        uint32_t maxInstanceCount = 0;
    };

    struct ShaderCapabilities
    {
        bool f16Supported = false;
        bool i8Supported = false;
    };
    
    void checkDeviceCapabilities();

    const MultiviewCapabilities& multiviewCapabilities() const;
    const ShaderCapabilities& shaderCapabilities() const;

private:
    vk::PhysicalDevice _handle = VK_NULL_HANDLE;

    vk::PhysicalDeviceMemoryProperties2 _memoryProperties{};
    std::vector<vk::ExtensionProperties> _deviceExtensions{};
    std::vector<vk::QueueFamilyProperties2> _queueProperties{};

    std::unordered_set<String> _enabledDeviceExtensions{};
    std::unordered_map<StringView, bool> _requestedDeviceExtensions{};

    MultiviewCapabilities _multiviewCaps{};
    ShaderCapabilities _shaderCaps{};

    // Queues
    std::optional<u32> _graphicsFamilyIndex;
    std::optional<u32> _computeFamilyIndex;
    std::optional<u32> _transferFamilyIndex;
    std::optional<u32> _sparseFamilyIndex;
    std::optional<u32> _presentationFamilyIndex;

    u32 _graphicsQueueCount = 0;
    u32 _computeQueueCount = 0;
    u32 _transferQueueCount = 0;
    u32 _sparseQueueCount = 0;
    u32 _presentationQueueCount = 0;
};
} // namespace bee