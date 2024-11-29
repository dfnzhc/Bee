/**
 * @File WindowsPlatform.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */
 
#include "Platform/Platform.hpp"

namespace bee {

void SetWindowIcon(const std::filesystem::path& path, WindowHandle windowHandle)
{
    // TODO
}

std::vector<StringView> SurfaceExtensions()
{
    u32 extCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);
    
    return std::vector<StringView>{glfwExtensions, glfwExtensions + extCount};
}

} // namespace bee
