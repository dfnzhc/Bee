/**
 * @File VK_DeviceDriver.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#include "Graphics/Vulkan/VK_DeviceDriver.hpp"
#include "Graphics/Vulkan/VK_Context.hpp"
#include "Graphics/Vulkan/VK_PhysicalDevice.hpp"
#include "Graphics/Vulkan/VK_Debug.hpp"
#include "Core/Common.hpp"

using namespace bee;

VK_DeviceDriver::VK_DeviceDriver(VK_Context* context) : _context(context)
{
}

VK_DeviceDriver::~VK_DeviceDriver()
{
}

Error VK_DeviceDriver::create(const Config& config)
{
    LogInfo("Creating 'Vulkan' device driver...");
    const auto devIdx = config.deviceIndex;

    _deviceInfo = _context->deviceInfo(devIdx);
    _frameCount = config.frameCount;

    _physicalDevice = std::make_unique<VK_PhysicalDevice>();
    BEE_REPORT_IF_FAILED(_physicalDevice->setup(_context->physicalDevice(devIdx)));

    BEE_REPORT_IF_FAILED(_initDeviceExtensions(config));
    BEE_REPORT_IF_FAILED(_initDevice());
    BEE_REPORT_IF_FAILED(_initAllocator());
    BEE_REPORT_IF_FAILED(_initPipelineCache());

    return Error::Ok;
}

void VK_DeviceDriver::destroy()
{
    _physicalDevice.reset();
    _device.waitIdle();
    _device.destroy();

    LogInfo("Vulkan device driver destroyed.");
}

Error VK_DeviceDriver::_initDeviceExtensions(const Config& config) const
{
    // extensions
    if (!config.headless) {
        _physicalDevice->registerExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, true);
    }

    if (config.raytracing) {
        const auto RayTracingExtensions = std::vector<std::string>{
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        };

        for (const auto& ext : RayTracingExtensions) {
            _physicalDevice->registerExtension(ext, true);
        }
    }

    _physicalDevice->finalizeRegister();
    _physicalDevice->checkDeviceCapabilities();

    return Error::Ok;
}

Error VK_DeviceDriver::_initDevice()
{
    _physicalDevice->requestQueues(vk::QueueFlagBits::eGraphics);
    const auto& familyIndices = _physicalDevice->availableQueueFamily();

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(familyIndices.size());
    std::vector<std::vector<float>> prioritiesForAllFamilies(familyIndices.size());
    for (u32 index = 0; const auto& [queueFamilyIndex, queueCount] : familyIndices) {
        prioritiesForAllFamilies[index] = std::vector<float>(queueCount, 1.0f);
        vk::DeviceQueueCreateInfo info;
        info.queueFamilyIndex = queueFamilyIndex;
        info.queueCount       = queueCount;
        info.pQueuePriorities = prioritiesForAllFamilies[index].data();
        queueCreateInfos.emplace_back(info);

        ++index;
    }

    const auto& extensions = _physicalDevice->enabledExtensions();
    const auto& layers     = _context->enabledLayers();

    vk::DeviceCreateInfo deviceInfo    = {};
    deviceInfo.pNext                   = nullptr;
    deviceInfo.queueCreateInfoCount    = static_cast<u32>(queueCreateInfos.size());
    deviceInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceInfo.enabledLayerCount       = static_cast<u32>(layers.size());
    deviceInfo.ppEnabledLayerNames     = layers.data();
    deviceInfo.enabledExtensionCount   = static_cast<u32>(extensions.size());
    deviceInfo.ppEnabledExtensionNames = extensions.data();

    _device = _physicalDevice->handle().createDevice(deviceInfo);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(_device);
    volkLoadDevice(_device);

    _debug = std::make_unique<VK_Debug>();
    _debug->setup(_device);
    _debug->setObjectName(_device, "Bee Device");

    return {};
}

Error VK_DeviceDriver::_initAllocator()
{
    return {};
}

Error VK_DeviceDriver::_initPipelineCache()
{
    return {};
}