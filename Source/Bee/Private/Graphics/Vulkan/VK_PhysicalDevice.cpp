#include "Graphics/Vulkan/VK_PhysicalDevice.hpp"
#include "Graphics/Vulkan/VK_Context.hpp"
#include "Core/Common.hpp"
#include "Core/Version.hpp"

using namespace bee;

Error VK_PhysicalDevice::setup(vk::PhysicalDevice physicalDevice)
{
    _handle = physicalDevice;
    _memoryProperties = physicalDevice.getMemoryProperties2();
    _queueProperties = physicalDevice.getQueueFamilyProperties2();

    return Error::Ok;
}

const std::vector<vk::QueueFamilyProperties2>& VK_PhysicalDevice::queueFamilyProperties() const
{
    return _queueProperties;
}

const vk::PhysicalDeviceMemoryProperties& VK_PhysicalDevice::memoryProperties() const
{
    return _memoryProperties.memoryProperties;
}

Result<u32> VK_PhysicalDevice::memoryType(u32 bits, vk::MemoryPropertyFlags properties) const
{
    const auto& memoryProperties = _memoryProperties.memoryProperties;
    for (u32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((bits & 1) == 1) {
            if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        bits >>= 1;
    }

    return Unexpected("No matching memory type found");
}

void VK_PhysicalDevice::registerExtension(StringView extName, bool required)
{
    // 如果扩展不是必要的，仅当未注册过该扩展，或之前注册的也是非必要的，该扩展才会被标记为非必要
    if (_requestedDeviceExtensions.contains(extName)) {
        auto& val = _requestedDeviceExtensions.at(extName);
        val |= required;

        return;
    }
    _requestedDeviceExtensions.emplace(extName, required);
}

void VK_PhysicalDevice::finalizeRegister()
{
    auto extensions = _handle.enumerateDeviceExtensionProperties();

    for (const auto& ext : extensions) {
        const auto* extName = ext.extensionName.data();
        if (_requestedDeviceExtensions.contains(extName)) {
            _enabledDeviceExtensions.emplace(extName);
        }
    }

    LogVerbose("\tRequested Vulkan device extensions：");
    for (const auto& ext : _requestedDeviceExtensions) {
        const auto extName = ext.first.data();
        const auto isRequested = ext.second;

        if (_enabledDeviceExtensions.contains(extName)) {
            LogVerbose("\t\tEnabled：{}.", extName);
        }
        else {
            if (isRequested) {
                LogError("\t\tNot Found[Req]：{}.", extName);
            }

            LogVerbose("\t\tNot Found[Opt]：{}.", extName);
        }
    }
}

std::vector<const char*> VK_PhysicalDevice::enabledExtensions() const
{
    std::vector<const char*> extensions;
    extensions.reserve(_enabledDeviceExtensions.size());
    for (auto& extension : _enabledDeviceExtensions) {
        extensions.push_back(extension.c_str());
    }

    return extensions;
}

bool VK_PhysicalDevice::isExtensionEnabled(StringView extName) const
{
    return _enabledDeviceExtensions.contains(extName);
}

void VK_PhysicalDevice::requestQueues(vk::QueueFlags flags, vk::SurfaceKHR surface)
{
    BEE_ASSERT(flags != vk::QueueFlags{});

    const auto& size = static_cast<u32>(_queueProperties.size());

    for (u32 queueFamilyIndex = 0; queueFamilyIndex < size; ++queueFamilyIndex) {
        const auto& props = _queueProperties[queueFamilyIndex].queueFamilyProperties;
        if (!_presentationFamilyIndex.has_value() && surface != VK_NULL_HANDLE) {
            if (_handle.getSurfaceSupportKHR(queueFamilyIndex, surface) == VK_TRUE) {
                _presentationFamilyIndex = queueFamilyIndex;
                _presentationQueueCount = props.queueCount;
            }
        }
        if (!_graphicsFamilyIndex.has_value() && (flags & props.queueFlags) & vk::QueueFlagBits::eGraphics) {
            _graphicsFamilyIndex = queueFamilyIndex;
            _graphicsQueueCount = props.queueCount;
            flags &= ~vk::QueueFlagBits::eGraphics;
            continue;
        }

        if (!_computeFamilyIndex.has_value() && (flags & props.queueFlags) & vk::QueueFlagBits::eCompute) {
            _computeFamilyIndex = queueFamilyIndex;
            _computeQueueCount = props.queueCount;
            flags &= ~vk::QueueFlagBits::eCompute;
            continue;
        }

        if (!_transferFamilyIndex.has_value() && (flags & props.queueFlags) & vk::QueueFlagBits::eTransfer) {
            _transferFamilyIndex = queueFamilyIndex;
            _transferQueueCount = props.queueCount;
            flags &= ~vk::QueueFlagBits::eTransfer;
            continue;
        }

        if (!_sparseFamilyIndex.has_value() && (flags & props.queueFlags) & vk::QueueFlagBits::eSparseBinding) {
            _sparseFamilyIndex = queueFamilyIndex;
            _sparseQueueCount = props.queueCount;
            flags &= ~vk::QueueFlagBits::eSparseBinding;
            continue;
        }
    }

    LogVerbose("\tAvailable Vulkan device queue：");
    if (_graphicsFamilyIndex.has_value())
        LogVerbose("\t\tGraphics queue：{}(cnt: {}).", _graphicsFamilyIndex.value(), _graphicsQueueCount);
    if (_computeFamilyIndex.has_value())
        LogVerbose("\t\tCompute queue：{}(cnt: {}).", _computeFamilyIndex.value(), _computeQueueCount);
    if (_transferFamilyIndex.has_value())
        LogVerbose("\t\tTransfer queue：{}(cnt: {}).", _transferFamilyIndex.value(), _transferQueueCount);
    if (_sparseFamilyIndex.has_value())
        LogVerbose("\t\tSparse binding queue：{}(cnt: {}).", _sparseFamilyIndex.value(), _sparseQueueCount);

    if (_presentationFamilyIndex.has_value()) {
        LogVerbose("\t\tPresent queue：{}(cnt: {}).", _presentationFamilyIndex.value(), _presentationQueueCount);
    }
    else if (surface != VK_NULL_HANDLE) {
        LogError("\tNo queues with presentation capabilities found.");
    }
}

std::vector<std::pair<u32, u32>> VK_PhysicalDevice::availableQueueFamily() const
{
    std::unordered_map<u32, u32> familyIndices;

    if (_graphicsFamilyIndex.has_value())
        familyIndices.emplace(_graphicsFamilyIndex.value(), graphicsFamilyCount());
    if (_computeFamilyIndex.has_value())
        familyIndices.emplace(_computeFamilyIndex.value(), computeFamilyCount());
    if (_transferFamilyIndex.has_value())
        familyIndices.emplace(_transferFamilyIndex.value(), transferFamilyCount());
    if (_sparseFamilyIndex.has_value())
        familyIndices.emplace(_sparseFamilyIndex.value(), sparseFamilyCount());
    if (_presentationFamilyIndex.has_value())
        familyIndices.emplace(_presentationFamilyIndex.value(), presentationFamilyCount());

    std::vector<std::pair<u32, u32>> queues;
    queues.reserve(familyIndices.size());

    for (const auto& [idx, cnt] : familyIndices) {
        queues.emplace_back(idx, cnt);
    }

    return queues;
}

std::optional<u32> VK_PhysicalDevice::graphicsFamilyIndex() const
{
    return _graphicsFamilyIndex;
}

std::optional<u32> VK_PhysicalDevice::computeFamilyIndex() const
{
    return _computeFamilyIndex;
}

std::optional<u32> VK_PhysicalDevice::transferFamilyIndex() const
{
    return _transferFamilyIndex;
}

std::optional<u32> VK_PhysicalDevice::sparseFamilyIndex() const
{
    return _sparseFamilyIndex;
}

std::optional<u32> VK_PhysicalDevice::presentationFamilyIndex() const
{
    return _presentationFamilyIndex;
}

u32 VK_PhysicalDevice::graphicsFamilyCount() const
{
    return _graphicsQueueCount;
}

u32 VK_PhysicalDevice::computeFamilyCount() const
{
    return _computeQueueCount;
}

u32 VK_PhysicalDevice::transferFamilyCount() const
{
    return _transferQueueCount;
}

u32 VK_PhysicalDevice::sparseFamilyCount() const
{
    return _sparseQueueCount;
}

u32 VK_PhysicalDevice::presentationFamilyCount() const
{
    return _presentationQueueCount;
}

bool VK_PhysicalDevice::isSupportPresent(vk::SurfaceKHR surface) const
{
    BEE_DEBUG_ASSERT(surface != VK_NULL_HANDLE, "Invalid surface");

    const auto& size = static_cast<u32>(_queueProperties.size());
    for (u32 queueFamilyIndex = 0; queueFamilyIndex < size; ++queueFamilyIndex) {
        if (_handle.getSurfaceSupportKHR(queueFamilyIndex, surface)) {
            return true;
        }
    }

    return false;
}

vk::PhysicalDevice VK_PhysicalDevice::handle() const
{
    return _handle;
}

VK_PhysicalDevice::operator vk::PhysicalDevice() const
{
    return _handle;
}

void VK_PhysicalDevice::checkDeviceCapabilities()
{
    const auto apiVersion = _handle.getProperties().apiVersion;

    const bool use13 = apiVersion >= VK_API_VERSION_1_3;
    const bool use12 = apiVersion >= VK_API_VERSION_1_2;
    const bool use11 = apiVersion >= VK_API_VERSION_1_1;

    // features
    {
        LogVerbose("\tCheck Vulkan device features：");

        VulkanChainList list;

        vk::PhysicalDeviceVulkan11Features features_11 = {};
        vk::PhysicalDeviceVulkan12Features features_12 = {};
        vk::PhysicalDeviceVulkan13Features features_13 = {};
        vk::PhysicalDeviceMultiviewFeatures multiviewFeatures = {};

        if (use11) {
            LogVerbose("\t\tAdd 'Vulkan11' Features.");
            list.insert(features_11);
        }
        if (use12) {
            LogVerbose("\t\tAdd 'Vulkan12' Features.");
            list.insert(features_12);
        }
        if (use13) {
            LogVerbose("\t\tAdd 'Vulkan13' Features.");
            list.insert(features_13);
        }

        if (isExtensionEnabled(VK_KHR_MULTIVIEW_EXTENSION_NAME)) {
            LogVerbose("\t\tAdd 'Multiview' Features.");
            list.insert(multiviewFeatures);
        }

        vk::PhysicalDeviceFeatures2 features2 = {};
        features2.pNext = list.head();
        _handle.getFeatures2(&features2);

        if (use12) {
            _shaderCaps.f16Supported = features_12.shaderFloat16;
            _shaderCaps.i8Supported = features_12.shaderInt8;
        }

        if (isExtensionEnabled(VK_KHR_MULTIVIEW_EXTENSION_NAME)) {
            _multiviewCaps.isSupported = multiviewFeatures.multiview;
            _multiviewCaps.geometryShaderSupported = multiviewFeatures.multiviewGeometryShader;
            _multiviewCaps.tessellationShaderSupported = multiviewFeatures.multiviewTessellationShader;
        }
    }
    
    LogVerbose("\t===============================");

    // properties
    {
        LogVerbose("\tCheck Vulkan device properties：");
        
        VulkanChainList list;
        vk::PhysicalDeviceMultiviewProperties multiviewProps = {};
        vk::PhysicalDeviceProperties2 properties2 = {};

        if (_multiviewCaps.isSupported) {
            LogVerbose("\t\tAdd 'Multiview' Properties.");
            list.insert(multiviewProps);
        }

        properties2.pNext = list.head();
        _handle.getProperties2(&properties2);

        if (_multiviewCaps.isSupported) {
            _multiviewCaps.maxViewCount = multiviewProps.maxMultiviewViewCount;
            _multiviewCaps.maxInstanceCount = multiviewProps.maxMultiviewInstanceIndex;
        }
    }

    // print verbose information
    {
        LogVerbose("\t\tShader Capabilities:");
        LogVerbose("\t\t  float 16:\t\t{}", _shaderCaps.f16Supported ? "Support" : "Not Support");
        LogVerbose("\t\t  int 8:\t\t{}", _shaderCaps.i8Supported ? "Support" : "Not Support");

        LogVerbose("\t\tMultiview Capabilities: {}", _multiviewCaps.isSupported ? "Support" : "Not Support");
        if (_multiviewCaps.isSupported) {
            LogVerbose("\t\t  supported:\t\t{}", _multiviewCaps.isSupported ? "True" : "False");
            LogVerbose("\t\t  geometry shader:\t\t{}", _multiviewCaps.geometryShaderSupported ? "Support" : "Not Support");
            LogVerbose("\t\t  tessellation shader:\t\t{}", _multiviewCaps.tessellationShaderSupported ? "Support" : "Not Support");
            LogVerbose("\t\t  max view count:\t\t{}", _multiviewCaps.maxViewCount);
            LogVerbose("\t\t  max instance count:\t\t{}", _multiviewCaps.maxInstanceCount);
        }
    }
}

const VK_PhysicalDevice::MultiviewCapabilities& VK_PhysicalDevice::multiviewCapabilities() const
{
    return _multiviewCaps;
}

const VK_PhysicalDevice::ShaderCapabilities& VK_PhysicalDevice::shaderCapabilities() const
{
    return _shaderCaps;
}