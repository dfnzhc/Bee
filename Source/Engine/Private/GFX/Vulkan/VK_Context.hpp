/**
 * @File VK_Context.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "GFX/GFX_Context.hpp"
#include <unordered_set>
#include <vulkan/vulkan.hpp>

namespace bee {
class BEE_API VK_Context final : public GFX_Context
{
public:
    ~VK_Context() override;

    // clang-format off
    Error create(const Config& config) override;
    void destroy() override;
    
    BEE_NODISCARD u32 deviceCount() const override;
    BEE_NODISCARD const DeviceInfo& deviceInfo(u32 devIdx) const override;
    
    BEE_NODISCARD UniquePtr<GFX_DeviceDriver> createDriver() override;
    // clang-format on

    BEE_NODISCARD vk::Instance instance() const;

public:
    BEE_NODISCARD vk::PhysicalDevice physicalDevice(u32 devIdx) const;
    BEE_NODISCARD u32 apiVersion() const;
    BEE_NODISCARD std::vector<const char*> enabledLayers() const;
    BEE_NODISCARD std::vector<const char*> enabledExtensions() const;
    
private:
    Error _initVulkanAPI();
    Error _initInstanceExtensions(const Config& config);
    Error _initInstance();
    Error _initPhysicalDevice();

    void _registerInstanceExtension(StringView extName, bool required = true);

    // clang-format off
    static VKAPI_ATTR VkBool32 VKAPI_CALL _debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT pMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT pMessageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    static VKAPI_ATTR VkBool32 VKAPI_CALL _debugReportCallback(VkDebugReportFlagsEXT pFlags, VkDebugReportObjectTypeEXT pObjectType, u64 pObject, size_t pLocation, i32 pMessageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData);
    // clang-format on

private:
    vk::Instance _instance = {};
    uint32_t _apiVersion   = vk::ApiVersion10;

    std::unordered_set<String> _enabledInstanceExtensions;
    std::unordered_map<StringView, bool> _requestedInstanceExtensions;

    std::vector<const char*> _enabledLayers;
    std::vector<const char*> _enabledExtensions;
    
    std::vector<DeviceInfo> _devices;
    std::vector<vk::PhysicalDevice> _physicalDevices;

    vk::DebugUtilsMessengerEXT _debugMessenger = nullptr;
    vk::DebugReportCallbackEXT _debugReport    = nullptr;
};
} // namespace bee