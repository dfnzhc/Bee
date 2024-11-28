/**
 * @File RenderContextVulkan.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#include "Render/Vulkan/RenderContextVulkan.hpp"
#include "Render/Vulkan/RenderDriverVulkan.hpp"

using namespace bee;

Error RenderContextVulkan::initialize()
{
    LogInfo("Vulkan 渲染上下文开始初始化...");
    
    // TODO: 初始化 Vulkan API，volk？
    // TODO：Extensions 和 Layers 设置
    // TODO：创建 Vulkan 实例，加载必要方法
    // TODO：枚举、搜集物理设备
    
    return Error::Ok;
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

