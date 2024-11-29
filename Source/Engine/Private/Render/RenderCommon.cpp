/**
 * @File RenderCommon.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */
 
#include "Render/RenderCommon.hpp"
#include "Render/Vulkan/RenderContextVulkan.hpp"

namespace bee {

std::unique_ptr<RenderContext> CreateRenderContext(RenderDeviceType type)
{
    if (type == RenderDeviceType::Vulkan)
        return std::make_unique<RenderContextVulkan>();

    BEE_UNREACHABLE();
    return nullptr;
}

} // namespace bee
