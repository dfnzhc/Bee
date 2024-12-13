/**
 * @File GFX.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#include "GFX/GFX.hpp"

#include "GFX/GFX_Device.hpp"
#include "GFX/Vulkan/VK_Context.hpp"

using namespace bee;

namespace bee {
void GFX_TEST()
{
    GFX_Desc desc;
    desc.gfxApi = GraphicsAPI::Vulkan;

    auto device = std::make_unique<GFX_Device>();
    BEE_ASSERT(device->create(desc) == Error::Ok);
    
    device->destroy();
}

std::unique_ptr<GFX_Context> CreateRenderContext(GraphicsAPI api)
{
    if (api == GraphicsAPI::Vulkan)
        return std::make_unique<VK_Context>();

    BEE_UNIMPLEMENTED();
    return nullptr;
}
} // namespace bee