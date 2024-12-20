#include "GFX/Vulkan/VK_PhysicalDevice.hpp"
#include "VK_Context.hpp"
#include "Base/Common.hpp"
#include <Core/Version.hpp>

using namespace bee;

VK_PhysicalDevice::VK_PhysicalDevice(u32 apiVersion, vk::PhysicalDevice physicalDevice) : _handle(physicalDevice)
{
    _memoryProperties = physicalDevice.getMemoryProperties2();
    _queueProperties  = physicalDevice.getQueueFamilyProperties2();

    const auto apiMajor = BEE_GET_VERSION_MAJOR(apiVersion), apiMinor = BEE_GET_VERSION_MINOR(apiVersion);

    // properties
    {
        VulkanChainList list;
        list.insert(_rayTracingPipelineProperties);
        list.insert(_fragmentDensityMapProperties);
        list.insert(_fragmentDensityMapOffsetProperties);
        list.insert(_properties11);

        if (apiMajor == 1 && apiMinor >= 2) {
            _properties12.driverID                     = vk::DriverId::eNvidiaProprietary;
            _properties12.supportedDepthResolveModes   = vk::ResolveModeFlagBits::eMax;
            _properties12.supportedStencilResolveModes = vk::ResolveModeFlagBits::eMax;
            list.insert(_properties12);
        }

        if (apiMajor == 1 && apiMinor >= 3) {
            list.insert(_properties13);
        }
        _properties.pNext = list.head();
        physicalDevice.getProperties2(&_properties);
    }

    // features
    {
        VulkanChainList list;
        list.insert(_bufferDeviceAddressFeatures);
        list.insert(_maintenance4Features);
        _features.pNext = list.head();
        physicalDevice.getFeatures2(&_features);

        _featureList.insert(_features11);

        if (apiMajor == 1 && apiMinor >= 2) {
            _features12.setDescriptorIndexing(true)
                .setRuntimeDescriptorArray(true)
                .setDescriptorBindingPartiallyBound(true)
                .setDescriptorBindingVariableDescriptorCount(true)
                .setTimelineSemaphore(true)
                .setShaderSampledImageArrayNonUniformIndexing(true)
                .setBufferDeviceAddress(_bufferDeviceAddressFeatures.bufferDeviceAddress);

            _featureList.insert(_properties12);
        }

        if (apiMajor == 1 && apiMinor >= 3) {
            _features13.setMaintenance4(_maintenance4Features.maintenance4);
            _featureList.insert(_properties13);
        }
    }
}

const vk::PhysicalDeviceFeatures& VK_PhysicalDevice::features() const
{
    return _requestedFeatures.features;
}

const vk::PhysicalDeviceProperties& VK_PhysicalDevice::properties() const
{
    return _properties.properties;
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

vk::PhysicalDeviceFeatures VK_PhysicalDevice::requestedFeatures() const
{
    return _requestedFeatures.features;
}

void* VK_PhysicalDevice::featureList() const
{
    return _featureList.head();
}

void VK_PhysicalDevice::registerExtension(StringView extName, bool required)
{
    // 如果扩展不是必要的，仅当未注册过该扩展，或之前注册的也是非必要的，该扩展才会被标记为非必要
    if (_requestedDeviceExtensions.contains(extName)) {
        auto& val  = _requestedDeviceExtensions.at(extName);
        val       |= required;

        return;
    }
    _requestedDeviceExtensions.emplace(extName, required);
}

std::vector<const char*> VK_PhysicalDevice::enabledExtensions()
{
    if (_enabledDeviceExtensions.empty()) {
        auto extensions = _handle.enumerateDeviceExtensionProperties();

        for (const auto& ext : extensions) {
            const auto* extName = ext.extensionName.data();
            if (_requestedDeviceExtensions.contains(extName)) {
                _enabledDeviceExtensions.emplace(extName);
            }
        }

        LogVerbose("\tRequested Vulkan device extensions：");
        for (const auto& ext : _requestedDeviceExtensions) {
            const auto extName     = ext.first.data();
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

        setupFeatures();
    }

    std::vector<const char*> extensions(_enabledDeviceExtensions.size());
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
                _presentationQueueCount  = props.queueCount;
            }
        }
        if (!_graphicsFamilyIndex.has_value() && (flags & props.queueFlags) & vk::QueueFlagBits::eGraphics) {
            _graphicsFamilyIndex  = queueFamilyIndex;
            _graphicsQueueCount   = props.queueCount;
            flags                &= ~vk::QueueFlagBits::eGraphics;
            continue;
        }

        if (!_computeFamilyIndex.has_value() && (flags & props.queueFlags) & vk::QueueFlagBits::eCompute) {
            _computeFamilyIndex  = queueFamilyIndex;
            _computeQueueCount   = props.queueCount;
            flags               &= ~vk::QueueFlagBits::eCompute;
            continue;
        }

        if (!_transferFamilyIndex.has_value() && (flags & props.queueFlags) & vk::QueueFlagBits::eTransfer) {
            _transferFamilyIndex  = queueFamilyIndex;
            _transferQueueCount   = props.queueCount;
            flags                &= ~vk::QueueFlagBits::eTransfer;
            continue;
        }

        if (!_sparseFamilyIndex.has_value() && (flags & props.queueFlags) & vk::QueueFlagBits::eSparseBinding) {
            _sparseFamilyIndex  = queueFamilyIndex;
            _sparseQueueCount   = props.queueCount;
            flags              &= ~vk::QueueFlagBits::eSparseBinding;
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

vk::PhysicalDevice VK_PhysicalDevice::handle() const
{
    return _handle;
}

VK_PhysicalDevice::operator vk::PhysicalDevice() const
{
    return _handle;
}

void VK_PhysicalDevice::setupFeatures()
{
    const auto accelStructSupported         = _enabledDeviceExtensions.contains(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    const auto rayPipelineSupported         = _enabledDeviceExtensions.contains(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    const auto rayQuerySupported            = _enabledDeviceExtensions.contains(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    const auto meshSupported                = _enabledDeviceExtensions.contains(VK_NV_MESH_SHADER_EXTENSION_NAME);
    const auto fragmentShadingRateSupported = _enabledDeviceExtensions.contains(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
    const auto synchronization2Supported    = _enabledDeviceExtensions.contains(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    const auto maintenance4Supported        = _enabledDeviceExtensions.contains(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);

    if (accelStructSupported) {
        _accelStructFeatures.setAccelerationStructure(true);
        _featureList.insert(_accelStructFeatures);
    }

    if (rayPipelineSupported) {
        _rayTracingPipelineFeatures.setRayTracingPipeline(true).setRayTraversalPrimitiveCulling(true);
        _featureList.insert(_rayTracingPipelineFeatures);
    }

    if (rayQuerySupported) {
        _rayQueryFeatures.setRayQuery(true);
        _featureList.insert(_rayQueryFeatures);
    }

    if (meshSupported) {
        _meshShaderFeature.setTaskShader(true).setMeshShader(true);
        _featureList.insert(_meshShaderFeature);
    }

    if (fragmentShadingRateSupported) {
        _fragmentShadingRateFeature.setPipelineFragmentShadingRate(true).setPrimitiveFragmentShadingRate(true).setAttachmentFragmentShadingRate(true);
        _featureList.insert(_fragmentShadingRateFeature);
    }

    if (synchronization2Supported) {
        _features13.setSynchronization2(synchronization2Supported);
    }

    if (maintenance4Supported && _features13.maintenance4) {
        _featureList.insert(_maintenance4Features);
    }
}