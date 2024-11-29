/**
 * @File RenderContextVulkan.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#include "Render/Vulkan/RenderContextVulkan.hpp"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "Render/Vulkan/RenderDriverVulkan.hpp"
#include "Render/RenderCommon.hpp"
#include "Base/Globals.hpp"
#include "Base/Common.hpp"

using namespace bee;

RenderContextVulkan::~RenderContextVulkan()
{
}

Error RenderContextVulkan::create(const Config& config)
{
    LogInfo("创建 Vulkan 渲染上下文...");

    // TODO: 初始化 Vulkan API，volk？
    // TODO：Extensions 和 Layers 设置
    // TODO：创建 Vulkan 实例，加载必要方法
    // TODO：枚举、搜集物理设备

    BEE_REPORT_IF_FAILED(_initVulkanAPI());
    BEE_REPORT_IF_FAILED(_initInstanceExtensions(config));

    return Error::Ok;
}

void RenderContextVulkan::destroy()
{
    LogInfo("Vulkan 渲染上下文已销毁");
}

const RenderContext::DeviceInfo& RenderContextVulkan::deviceInfo(u32 devIdx) const
{
    static DeviceInfo info;
    return info;
}

u32 RenderContextVulkan::deviceCount() const
{
    return 0;
}

bool RenderContextVulkan::deviceSupportsPresent(u32 devIdx) const
{
    return false;
}

std::unique_ptr<RenderDriver> RenderContextVulkan::createDriver()
{
    return std::make_unique<RenderDriverVulkan>(this);
}

Error RenderContextVulkan::_initVulkanAPI()
{
    auto vkGetInstanceProcAddr = vk::DynamicLoader().getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    
    auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));

    if (FN_vkEnumerateInstanceVersion == nullptr) {
        _apiVersion = VK_API_VERSION_1_0;
        LogWarn("'vkEnumerateInstanceVersion' 不可用，假设 Vulkan 版本为 '1.0'");
    }
    else {
        auto result = FN_vkEnumerateInstanceVersion(&_apiVersion);
        if (result != VK_SUCCESS)
            return Error::VulkanError;
    }

    return Error::Ok;
}

Error RenderContextVulkan::_initInstanceExtensions(const Config& config)
{
    _enabledInstanceExtensions.clear();

    if (!config.headless) {
        _registerInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME, true);

        for (auto ext : SurfaceExtensions())
            _registerInstanceExtension(ext, true);
    }

    if (Globals::EnableValidationLayer()) {
        _registerInstanceExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, false);
        _registerInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false);
    }

    _registerInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, false);

    const auto& extensions = vk::enumerateInstanceExtensionProperties();
    for (const auto& ext : extensions) {
        const auto* extName = ext.extensionName.data();
        if (_requestedInstanceExtensions.contains(extName)) {
            _enabledInstanceExtensions.insert(extName);
        }
    }

    LogVerbose("\t请求的 Vulkan 实例扩展有：");
    for (const auto& ext : _requestedInstanceExtensions) {
        const auto extName     = ext.first.data();
        const auto isRequested = ext.second;

        if (_enabledInstanceExtensions.contains(extName)) {
            LogVerbose("\t\t已添加：{}", extName);
        }
        else {
            if (isRequested) {
                LogError("\t\t未找到必要扩展：{}", extName);
                return Error::CheckError;
            }
            else {
                LogVerbose("\t\t未找到可选扩展：{}", extName);
            }
        }
    }

    return Error::Ok;
}

Error RenderContextVulkan::_initInstance()
{
    return Error::Ok;
}

Error RenderContextVulkan::_initPhysicalDevice()
{
    return Error::Ok;
}

void RenderContextVulkan::_registerInstanceExtension(StringView extName, bool required)
{
    // 如果扩展不是必要的，仅当未注册过该扩展，或之前注册的也是非必要的，该扩展才会被标记为非必要
    if (_requestedInstanceExtensions.contains(extName)) {
        auto& val  = _requestedInstanceExtensions.at(extName);
        val       |= required;

        return;
    }

    _requestedInstanceExtensions.emplace(extName, required);
}
