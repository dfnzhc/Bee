/**
 * @File VK_DeviceDriver.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Graphics/GFX_Context.hpp"
#include "Graphics/GFX_DeviceDriver.hpp"
#include "Memory/Memory.hpp"

#include "Graphics/Vulkan/VK_Common.hpp"

// TODO: 创建 Vulkan RHI 的单元测试

namespace bee {
class VK_Debug;
class VK_Context;
class VK_PhysicalDevice;

class BEE_API VK_DeviceDriver final : public GFX_DeviceDriver
{
public:
    VK_DeviceDriver(VK_Context* context);
    ~VK_DeviceDriver() override;

    Error create(const Config& config) override;
    void destroy() override;

private:
    Error _initDeviceExtensions(const Config& config) const;
    Error _initDevice();
    Error _initAllocator();
    Error _initPipelineCache();

private:
    VK_Context* _context = nullptr;
    vk::Device _device   = VK_NULL_HANDLE;

    UniquePtr<VK_PhysicalDevice> _physicalDevice;
    UniquePtr<VK_Debug> _debug;

    GFX_Context::DeviceInfo _deviceInfo = {};

    uint32_t _frameCount = 1;
};
} // namespace bee