/**
 * @File PlatformHandles.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Portability.hpp>

#define GLFW_INCLUDE_NONE

#ifdef BEE_IN_WINDOWS
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3.h>
#  include <GLFW/glfw3native.h>
#endif

namespace bee {

#ifdef BEE_IN_WINDOWS
using SharedLibraryHandle = HANDLE;
using WindowHandle        = HWND;
using WindowApiHandle     = GLFWwindow*;
#endif

} // namespace bee