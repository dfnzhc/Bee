/**
 * @File GFX.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Graphics/RHI/RHI_Common.hpp"

namespace bee {

class GFX_Context;

struct GFX_Desc
{
    GraphicsAPI gfxApi = GraphicsAPI::Unknown;
    
};


void GFX_TEST();

BEE_API std::unique_ptr<GFX_Context> CreateRenderContext(GraphicsAPI api);

} // namespace bee