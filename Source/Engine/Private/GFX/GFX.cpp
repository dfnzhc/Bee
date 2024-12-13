/**
 * @File GFX.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */
 
#include "GFX/GFX.hpp"
#include "GFX/Vulkan/VK_Context.hpp"

namespace bee {

UniquePtr<GFX_Context> CreateRenderContext(RenderDeviceType type)
{
    if (type == RenderDeviceType::Vulkan)
        return std::make_unique<VK_Context>();

    BEE_UNREACHABLE();
    return nullptr;
}

} // namespace bee
