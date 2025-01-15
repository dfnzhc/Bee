/**
 * @File PlatformHandles.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Portability.hpp"

#define GLFW_INCLUDE_NONE

#ifdef BEE_IN_WINDOWS
#  define SDL_MAIN_HANDLED
#  include <SDL3/SDL.h>
#endif

namespace bee {

#ifdef BEE_IN_WINDOWS
using SharedLibraryHandle = HANDLE;
using WindowHandle        = HWND;
using WindowApiHandle     = SDL_Window*;
#endif

} // namespace bee