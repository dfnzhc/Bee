/**
 * @File VK_DeviceDriver.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#include "GFX/Vulkan/VK_DeviceDriver.hpp"

using namespace bee;

VK_DeviceDriver::VK_DeviceDriver(VK_Context* context) : _context(context)
{
}

VK_DeviceDriver::~VK_DeviceDriver()
{
    
}

Error VK_DeviceDriver::create(u32 deviceIndex, u32 frameCount)
{
    LogInfo("创建 Vulkan 渲染驱动...");
    // TODO：从 context 中获取物理设备 & 队列信息
    // TODO：Extensions 和 Layers
    // TODO：设备特性与 Capabilities
    // TODO：创建设备
    // TODO：创建分配器 vma，自定义缓冲池等
    // TODO：创建 pipeline cache
    
    
    return Error::Ok;
}

void VK_DeviceDriver::destroy()
{
    LogInfo("Vulkan 渲染驱动已摧毁");
}