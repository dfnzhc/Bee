/**
 * @File VK_Context.cpp
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
#include "GFX/Vulkan/VK_Common.hpp"
#include "GFX/Vulkan/VK_Context.hpp"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "GFX/Vulkan/VK_DeviceDriver.hpp"
#include "GFX/GFX.hpp"
#include "Base/Globals.hpp"
#include "Base/Common.hpp"
#include <Core/Version.hpp>
#include <algorithm>

using namespace bee;

VK_Context::~VK_Context()
{
    if (_debugMessenger != nullptr) {
        _instance.destroyDebugUtilsMessengerEXT(_debugMessenger);
        _debugMessenger = nullptr;
    }

    if (_debugReport != nullptr) {
        _instance.destroyDebugReportCallbackEXT(_debugReport);
        _debugReport = nullptr;
    }

    if (_instance != VK_NULL_HANDLE) {
        _instance.destroy();
    }
}

Error VK_Context::create(const Config& config)
{
    LogInfo("Creating 'Vulkan' render context...");

    BEE_REPORT_IF_FAILED(_initVulkanAPI());
    BEE_REPORT_IF_FAILED(_initInstanceExtensions(config));
    BEE_REPORT_IF_FAILED(_initInstance());
    BEE_REPORT_IF_FAILED(_initPhysicalDevice());

    return Error::Ok;
}

void VK_Context::destroy()
{
    LogInfo("Vulkan has been destroyed.");
}

const GFX_Context::DeviceInfo& VK_Context::deviceInfo(u32 devIdx) const
{
    BEE_DEBUG_ASSERT(devIdx < _physicalDevices.size());
    return _devices[devIdx];
}

u32 VK_Context::deviceCount() const
{
    return static_cast<u32>(_devices.size());
}

bool VK_Context::deviceSupportsPresent(u32 devIdx) const
{
    return {};
}

UniquePtr<GFX_DeviceDriver> VK_Context::createDriver()
{
    return std::make_unique<VK_DeviceDriver>(this);
}

vk::Instance VK_Context::instance() const
{
    return _instance;
}

vk::PhysicalDevice VK_Context::physicalDevice(u32 devIdx) const
{
    BEE_DEBUG_ASSERT(devIdx < _physicalDevices.size());

    return _physicalDevices[devIdx];
}

u32 VK_Context::apiVersion() const
{
    return _apiVersion;
}

std::vector<const char*> VK_Context::enabledLayers() const
{
    return _enabledLayers;
}

Error VK_Context::_initVulkanAPI()
{
    if (volkInitialize() != VK_SUCCESS) {
        LogError("Volk initialize failed.");
        return Error::VulkanError;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::DynamicLoader().getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

    auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (!FN_vkEnumerateInstanceVersion || VK_SUCCESS != FN_vkEnumerateInstanceVersion(&_apiVersion)) {
        _apiVersion = VK_API_VERSION_1_0;
        LogWarn("'vkEnumerateInstanceVersion' not available，default Vulkan version is '1.0'.");
    }

    LogVerbose("\tSelected Vulkan version: '{}.{}.{}'.", VK_VERSION_MAJOR(_apiVersion), VK_VERSION_MINOR(_apiVersion), VK_VERSION_PATCH(_apiVersion));

    return Error::Ok;
}

Error VK_Context::_initInstanceExtensions(const Config& config)
{
    _enabledInstanceExtensions.clear();

    if (!config.headless) {
        _registerInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME, true);

        // TODO: window api extensions
        // for (auto ext : SurfaceExtensions())
            // _registerInstanceExtension(ext, true);
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

    LogVerbose("\tRequested Vulkan instance extensions：");
    for (const auto& ext : _requestedInstanceExtensions) {
        const auto extName     = ext.first.data();
        const auto isRequested = ext.second;

        if (_enabledInstanceExtensions.contains(extName)) {
            LogVerbose("\t\tEnabled：{}.", extName);
        }
        else {
            if (isRequested) {
                LogError("\t\tNot Found[Req]：{}.", extName);
                return Error::CheckError;
            }

            LogVerbose("\t\tNot Found[Opt]：{}.", extName);
        }
    }

    return Error::Ok;
}

Error VK_Context::_initInstance()
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

    std::vector<const char*>().swap(_enabledLayers);
    if (Globals::EnableValidationLayer()) {
        const auto& layers = vk::enumerateInstanceLayerProperties();

        if (std::ranges::find_if(layers, [](const vk::LayerProperties& props) {
                return std::strcmp(props.layerName, "VK_LAYER_KHRONOS_validation") == 0;
            }) != layers.end())
        {
            _enabledLayers.emplace_back("VK_LAYER_KHRONOS_validation");
        }
    }

    LogVerbose("\tEnabled Vulkan layers：");
    for (const auto& layer : _enabledLayers) {
        LogVerbose("\t\tEnabled：{}.", layer);
    }

    vk::InstanceCreateInfo instanceInfo  = {};
    instanceInfo.pApplicationInfo        = &appInfo;
    instanceInfo.enabledExtensionCount   = enabledExtensions.size();
    instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();
    instanceInfo.enabledLayerCount       = _enabledLayers.size();
    instanceInfo.ppEnabledLayerNames     = _enabledLayers.data();

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo      = {};
    vk::DebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};

    const auto hasDebugUtilsExtension  = _enabledInstanceExtensions.contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    const auto hasDebugReportExtension = _enabledInstanceExtensions.contains(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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
        LogError("Failed to create Vulkan instance.");
        return Error::VulkanError;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);
    volkLoadInstance(_instance);

    if (hasDebugUtilsExtension) {
        auto result = _instance.createDebugUtilsMessengerEXT(&debugMessengerInfo, nullptr, &_debugMessenger);
        if (result != vk::Result::eSuccess) {
            LogWarn("Failed to Create Debug Utils Messenger: {}", vk::to_string(result));
            return Error::VulkanError;
        }
        LogVerbose("\tDebug Utils Messenger Created.");
    }
    else if (hasDebugReportExtension) {
        auto result = _instance.createDebugReportCallbackEXT(&debugReportCallbackInfo, nullptr, &_debugReport);
        if (result != vk::Result::eSuccess) {
            LogWarn("Failed to Create Debug Report Callback: {}", vk::to_string(result));
            return Error::VulkanError;
        }
        LogVerbose("\tDebug Report Callback Created.");
    }

    return Error::Ok;
}

Error VK_Context::_initPhysicalDevice()
{
    const auto physicalDevices = _instance.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        LogError("No available Vulkan physical device.");
        return Error::VulkanError;
    }

    const auto devCount = physicalDevices.size();

    std::vector<DeviceInfo>(devCount).swap(_devices);
    std::vector<vk::PhysicalDevice>(devCount).swap(_physicalDevices);

    LogVerbose("\t'{}' physical devices founded：", devCount);
    for (size_t i = 0u; i < devCount; ++i) {
        const auto& dev = physicalDevices[i];
        auto props      = dev.getProperties();

        auto& devInfo  = _devices[i];
        devInfo.name   = props.deviceName.data();
        devInfo.vendor = me::enum_cast<Vendor>(props.vendorID).value_or(Vendor::Unknown);
        devInfo.type   = me::enum_cast<DeviceType>(me::enum_integer(props.deviceType)).value_or(DeviceType::Other);

        _physicalDevices[i] = dev;

        LogVerbose("\t\t [{}]: {}.", i + 1, devInfo.name);
    }

    return Error::Ok;
}

void VK_Context::_registerInstanceExtension(StringView extName, bool required)
{
    // 如果扩展不是必要的，仅当未注册过该扩展，或之前注册的也是非必要的，该扩展才会被标记为非必要
    if (_requestedInstanceExtensions.contains(extName)) {
        auto& val  = _requestedInstanceExtensions.at(extName);
        val       |= required;

        return;
    }

    _requestedInstanceExtensions.emplace(extName, required);
}

VKAPI_ATTR VkBool32 VKAPI_CALL VK_Context::_debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT pMessageSeverity,
                                                                   VkDebugUtilsMessageTypeFlagsEXT pMessageType,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                   void* pUserData)
{
    String msg{pCallbackData->pMessage};

    // skipped
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
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)                                                      : type = "General"; break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)                                                   : type = "Validation"; break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)                                                  : type = "Performance"; break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) : type = "Validation|Performance"; break;
    default                                                                                                 : BEE_UNREACHABLE(); break;
    }

    String objects;
    if (pCallbackData->objectCount > 0) {
        objects = fmt::format("\n\t'{}' Objects：", pCallbackData->objectCount);
        for (u32 object = 0; object < pCallbackData->objectCount; ++object) {
            objects += fmt::format("\n\t\tObject[{}] - {}, Handle {}",
                                   object,
                                   me::enum_name(vk::ObjectType(pCallbackData->pObjects[object].objectType)),
                                   fmt::to_string(pCallbackData->pObjects[object].objectHandle));
            if (pCallbackData->pObjects[object].pObjectName && std::strlen(pCallbackData->pObjects[object].pObjectName) > 0) {
                objects += fmt::format(", name: \"{}\"", pCallbackData->pObjects[object].pObjectName);
            }
        }
    }

    String labels_string;
    String labels;
    if (pCallbackData->cmdBufLabelCount > 0) {
        labels = fmt::format("\n\t'{}' Command Buffer Labels：", pCallbackData->cmdBufLabelCount);
        for (u32 label = 0; label < pCallbackData->cmdBufLabelCount; ++label) {
            labels += fmt::format("\n\t\tLabel[{}] - {}{{ ", label, pCallbackData->pCmdBufLabels[label].pLabelName);
            for (int idx = 0; idx < 4; ++idx) {
                labels += fmt::format("{}", pCallbackData->pCmdBufLabels[label].color[idx]);
                if (idx < 3) {
                    labels += ", ";
                }
            }
            labels += " }";
        }
    }

    const auto errMsg = std::format("{} - Message ID：number-'{}' | name-'{}'\n\t{}{}{}",
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
        LogError(errMsg);
        // TODO：aborting？
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT : break;
    default                                                     : BEE_UNREACHABLE(); break;
    }

    return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VK_Context::_debugReportCallback(VkDebugReportFlagsEXT pFlags,
                                                                VkDebugReportObjectTypeEXT pObjectType,
                                                                u64 pObject,
                                                                size_t pLocation,
                                                                i32 pMessageCode,
                                                                const char* pLayerPrefix,
                                                                const char* pMessage,
                                                                void* pUserData)
{
    const auto dbgMsg = std::format("Vulkan Debug Report：'{}' Objects：\n{}", pObject, pMessage);

    switch (pFlags) {
    case VK_DEBUG_REPORT_DEBUG_BIT_EXT :
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT         : LogInfo(dbgMsg); break;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT             :
    case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT : LogWarn(dbgMsg); break;
    case VK_DEBUG_REPORT_ERROR_BIT_EXT               : LogError(dbgMsg); break;
    default                                          : BEE_UNREACHABLE();
        break;
    }

    return VK_FALSE;
}