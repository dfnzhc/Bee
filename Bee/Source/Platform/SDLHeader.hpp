/**
 * @File SDLHeader.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/31
 * @Brief This file is part of Bee.
 */

#pragma once

#include <SDL3/SDL.h>
#include "Bee/Core/Log.hpp"

namespace bee
{

#define BEE_SDL_CALL(expr)                                                      \
    ({                                                                          \
        auto _result = (expr);                                                  \
        if (!_result) { BEE_ERROR("{} 调用失败: {}.", #expr, SDL_GetError()); } \
        (_result);                                                              \
    })

} // namespace bee
