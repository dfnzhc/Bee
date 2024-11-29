/**
 * @File RenderContextVulkan.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Render/RenderContext.hpp"

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#  define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include <vulkan/vulkan.hpp>
#include <unordered_set>

namespace bee {

class BEE_API RenderContextVulkan final : public RenderContext
{
public:
    ~RenderContextVulkan() override;

    // clang-format off
    Error create(const Config& config) override;
    void destroy() override;
    
    BEE_NODISCARD const DeviceInfo& deviceInfo(u32 devIdx) const override;
    BEE_NODISCARD u32 deviceCount() const override;
    BEE_NODISCARD bool deviceSupportsPresent(u32 devIdx) const override;

    BEE_NODISCARD std::unique_ptr<RenderDriver> createDriver() override;
    // clang-format on

private:
    Error _initVulkanAPI();
    Error _initInstanceExtensions(const Config& config);
    Error _initInstance();
    Error _initPhysicalDevice();
    
    void _registerInstanceExtension(StringView extName, bool required = true);

private:
    vk::Instance _instance = {};
    uint32_t _apiVersion = vk::ApiVersion10; 

    std::unordered_set<StringView> _enabledInstanceExtensions;
    std::unordered_map<StringView, bool> _requestedInstanceExtensions;
};

} // namespace bee