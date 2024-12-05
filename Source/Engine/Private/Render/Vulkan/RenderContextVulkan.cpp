/**
 * @File RenderContextVulkan.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#ifndef VK_NO_PROTOTYPES
#  define VK_NO_PROTOTYPES
#endif
#ifndef VOLK_IMPLEMENTATION
#  define VOLK_IMPLEMENTATION
#endif
#include "Render/Vulkan/VulkanCommon.hpp"
#include "Render/Vulkan/RenderContextVulkan.hpp"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "Render/Vulkan/RenderDriverVulkan.hpp"
#include "Render/RenderCommon.hpp"
#include "Base/Globals.hpp"
#include "Base/Common.hpp"
#include <Core/Version.hpp>
#include <algorithm>

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
    BEE_REPORT_IF_FAILED(_initInstance());
    BEE_REPORT_IF_FAILED(_initPhysicalDevice());

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

UniquePtr<RenderDriver> RenderContextVulkan::createDriver()
{
    return std::make_unique<RenderDriverVulkan>(this);
}

Error RenderContextVulkan::_initVulkanAPI()
{
    if (volkInitialize() != VK_SUCCESS) {
        LogError("Volk 初始化失败");
        return Error::VulkanError;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::DynamicLoader().getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

    auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (!FN_vkEnumerateInstanceVersion || VK_SUCCESS != FN_vkEnumerateInstanceVersion(&_apiVersion)) {
        _apiVersion = VK_API_VERSION_1_0;
        LogWarn("'vkEnumerateInstanceVersion' 不可用，假设 Vulkan 版本为 '1.0'");
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
            _enabledInstanceExtensions.emplace(extName);
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
    vk::ApplicationInfo appInfo = {};
    appInfo.pApplicationName    = "Bee";
    appInfo.pEngineName         = "BeeEngine";
    appInfo.engineVersion       = VK_MAKE_VERSION(BEE_VERSION_MAJOR, BEE_VERSION_MINOR, BEE_VERSION_PATCH);
    appInfo.apiVersion          = _apiVersion;

    std::vector<const char*> enabledExtensions;
    enabledExtensions.reserve(_enabledInstanceExtensions.size());
    for (const auto& ext : _enabledInstanceExtensions) {
        enabledExtensions.emplace_back(ext.data());
    }

    std::vector<const char*> enabledLayers;
    if (Globals::EnableValidationLayer()) {
        const auto& layers = vk::enumerateInstanceLayerProperties();

        if (std::ranges::find_if(layers, [](const vk::LayerProperties& props) {
                return std::strcmp(props.layerName, "VK_LAYER_KHRONOS_validation") == 0;
            }) != layers.end())
        {
            enabledLayers.emplace_back("VK_LAYER_KHRONOS_validation");
        }
    }

    LogVerbose("\t开启的 Vulkan 层有：");
    for (const auto& layer : enabledLayers) {
        LogVerbose("\t\t已添加：{}", layer);
    }

    vk::InstanceCreateInfo instanceInfo  = {};
    instanceInfo.pApplicationInfo        = &appInfo;
    instanceInfo.enabledExtensionCount   = enabledExtensions.size();
    instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();
    instanceInfo.enabledLayerCount       = enabledLayers.size();
    instanceInfo.ppEnabledLayerNames     = enabledLayers.data();

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo      = {};
    vk::DebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};
    const auto hasDebugUtilsExtension                            = _enabledInstanceExtensions.contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    const auto hasDebugReportExtension                           = _enabledInstanceExtensions.contains(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    // clang-format off
    if (hasDebugUtilsExtension) {
        debugMessengerInfo.pNext = nullptr;
        debugMessengerInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
        debugMessengerInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        debugMessengerInfo.pfnUserCallback = _debugMessengerCallback;
        debugMessengerInfo.pUserData = this;
        instanceInfo.pNext = &debugMessengerInfo;
    } else if (hasDebugReportExtension) {
        debugReportCallbackInfo.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::eDebug;
        debugReportCallbackInfo.pfnCallback = _debugReportCallback;
        debugReportCallbackInfo.pUserData = this;
        instanceInfo.pNext = &debugReportCallbackInfo;
    }
    // clang-format on

    _instance = vk::createInstance(instanceInfo);
    if (_instance == VK_NULL_HANDLE) {
        LogError("创建 Vulkan 实例失败");
        return Error::VulkanError;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);
    volkLoadInstance(_instance);

    return Error::Ok;
}

Error RenderContextVulkan::_initPhysicalDevice()
{
    const auto physicalDevices = _instance.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        LogError("无可用的 Vulkan 物理设备");
        return Error::VulkanError;
    }

    const auto devCount = physicalDevices.size();

    std::vector<DeviceInfo>(devCount).swap(_devices);
    std::vector<vk::PhysicalDevice>(devCount).swap(_physicalDevices);
    std::vector<DeviceQueueFamilies>(devCount).swap(_deviceQueueFamilies);
    
    LogVerbose("\t发现 '{}' 个可用的物理设备：", devCount);
    for (size_t i = 0u; i < devCount; ++i) {
        const auto& dev = physicalDevices[i];
        auto props      = dev.getProperties();

        auto& devInfo  = _devices[i];
        devInfo.name   = props.deviceName.data();
        devInfo.vendor = Vendor(props.vendorID);
        devInfo.type   = DeviceType(props.deviceType);
        
        LogVerbose("\t\t [{}]: {}", i + 1, devInfo.name);
        
        _deviceQueueFamilies[i].properties = dev.getQueueFamilyProperties();
        _physicalDevices[i] = dev;
    }

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

VKAPI_ATTR VkBool32 VKAPI_CALL RenderContextVulkan::_debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT pMessageSeverity,
                                                                            VkDebugUtilsMessageTypeFlagsEXT pMessageType,
                                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                            void* pUserData)
{
    String msg{pCallbackData->pMessage};

    // 需要忽略的消息
    if (msg.contains("Mapping an image with layout") && msg.contains("can result in undefined behavior if this memory is used by the device")) {
        return VK_FALSE;
    }
    if (msg.contains("Invalid SPIR-V binary version 1.3")) {
        return VK_FALSE;
    }
    if (msg.contains("Shader requires flag")) {
        return VK_FALSE;
    }
    if (msg.contains("SPIR-V module not valid: Pointer operand") && msg.contains("must be a memory object")) {
        return VK_FALSE;
    }
    if (pCallbackData->pMessageIdName && String{pCallbackData->pMessageIdName}.contains("UNASSIGNED-CoreValidation-DrawState-ClearCmdBeforeDraw")) {
        return VK_FALSE;
    }

    String type;
    switch (pMessageType) {
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)                                                      : type = "常规"; break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)                                                   : type = "验证"; break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)                                                  : type = "性能"; break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) : type = "验证|性能"; break;
    default                                                                                                 : BEE_UNREACHABLE(); break;
    }

    String objects;
    if (pCallbackData->objectCount > 0) {
        objects = fmt::format("\n\t存在 '{}' 个对象：", pCallbackData->objectCount);
        for (u32 object = 0; object < pCallbackData->objectCount; ++object) {
            objects += fmt::format("\n\t\t对象[{}] - {}, Handle {}",
                                   object,
                                   me::enum_name(vk::ObjectType(pCallbackData->pObjects[object].objectType)),
                                   fmt::to_string(pCallbackData->pObjects[object].objectHandle));
            if (pCallbackData->pObjects[object].pObjectName && std::strlen(pCallbackData->pObjects[object].pObjectName) > 0) {
                objects += fmt::format(", 名称 \"{}\"", pCallbackData->pObjects[object].pObjectName);
            }
        }
    }

    String labels_string;
    String labels;
    if (pCallbackData->cmdBufLabelCount > 0) {
        labels = fmt::format("\n\t存在 '{}' 个命令缓冲区标签：", pCallbackData->cmdBufLabelCount);
        for (u32 label = 0; label < pCallbackData->cmdBufLabelCount; ++label) {
            labels += fmt::format("\n\t\t标签[{}] - {}{{ ", label, pCallbackData->pCmdBufLabels[label].pLabelName);
            for (int idx = 0; idx < 4; ++idx) {
                labels += fmt::format("{}", pCallbackData->pCmdBufLabels[label].color[idx]);
                if (idx < 3) {
                    labels += ", ";
                }
            }
            labels += " }";
        }
    }

    const auto errMsg = std::format("{} - 消息索引与名称为：{} | {}\n\t{}{}{}",
                                    type.data(),
                                    pCallbackData->messageIdNumber,
                                    pCallbackData->pMessageIdName,
                                    pCallbackData->pMessage,
                                    objects.data(),
                                    labels.data());

    // Convert VK severity to our own log macros.
    switch (pMessageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT : LogVerbose(errMsg); break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    : LogInfo(errMsg); break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : LogWarn(errMsg); break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT :
        LogError(errMsg.data());
        // TODO：GPU 错误直接退出程序？
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT : break;
    default                                                     : BEE_UNREACHABLE(); break;
    }

    return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL RenderContextVulkan::_debugReportCallback(VkDebugReportFlagsEXT pFlags,
                                                                         VkDebugReportObjectTypeEXT pObjectType,
                                                                         u64 pObject,
                                                                         size_t pLocation,
                                                                         i32 pMessageCode,
                                                                         const char* pLayerPrefix,
                                                                         const char* pMessage,
                                                                         void* pUserData)
{
    const auto dbgMsg = std::format("Vulkan 调试报告：有 '{}' 个对象：\n{}", pObject, pMessage);

    switch (pFlags) {
    case VK_DEBUG_REPORT_DEBUG_BIT_EXT :
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT         : LogInfo(dbgMsg); break;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT             :
    case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT : LogWarn(dbgMsg); break;
    case VK_DEBUG_REPORT_ERROR_BIT_EXT               : LogError(dbgMsg); break;
    default                                          : BEE_UNREACHABLE(); break;
    }

    return VK_FALSE;
}