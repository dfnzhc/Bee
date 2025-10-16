/**
 * @File WindowManager.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/18
 * @Brief This file is part of Bee.
 */

#include "./WindowManager.hpp"

using namespace Bee;

WindowManager::WindowManager()
{
}

WindowManager::~WindowManager()
{
}

bool WindowManager::initialize()
{
    return {};
}

void WindowManager::shutdown()
{
}

bool WindowManager::isInitialized() const
{
    return {};
}

WindowHandle WindowManager::createWindow(const WindowCreateInfo& createInfo)
{
    return WindowHandle{};
}

void WindowManager::destroyWindow(WindowHandle window)
{
}

bool WindowManager::isWindowValid(WindowHandle window) const
{
    return {};
}

bool WindowManager::setWindowTitle(WindowHandle window, const std::string& title)
{
    return {};
}

bool WindowManager::setWindowPosition(WindowHandle window, Vec2u pos)
{
    return {};
}

bool WindowManager::setWindowSize(WindowHandle window, Vec2u size)
{
    return {};
}

std::string WindowManager::GetWindowTitle(WindowHandle window) const
{
    return {};
}

Vec2u WindowManager::GetWindowPosition(WindowHandle window) const
{
    return {};
}

Vec2u WindowManager::GetWindowSize(WindowHandle window) const
{
    return {};
}

bool WindowManager::showWindow(WindowHandle window)
{
    return {};
}

bool WindowManager::hideWindow(WindowHandle window)
{
    return {};
}

bool WindowManager::minimizeWindow(WindowHandle window)
{
    return {};
}

bool WindowManager::maximizeWindow(WindowHandle window)
{
    return {};
}

bool WindowManager::restoreWindow(WindowHandle window)
{
    return {};
}

bool WindowManager::setWindowFullscreen(WindowHandle window, bool fullscreen)
{
    return {};
}

bool WindowManager::focusWindow(WindowHandle window)
{
    return {};
}

bool WindowManager::isWindowFocused(WindowHandle window) const
{
    return {};
}

WindowState WindowManager::getWindowState(WindowHandle window) const
{
    return {};
}

std::vector<WindowHandle> WindowManager::GetAllWindows() const
{
    return {};
}

WindowHandle WindowManager::GetMainWindow() const
{
    return WindowHandle{};
}

WindowHandle WindowManager::GetFocusedWindow() const
{
    return WindowHandle{};
}

NativeWindowHandle WindowManager::getNativeWindowHandle(WindowHandle window) const
{
    return {};
}

std::vector<DisplayInfo> WindowManager::getDisplays() const
{
    return {};
}

DisplayInfo WindowManager::getPrimaryDisplay() const
{
    return {};
}

DisplayInfo WindowManager::getWindowDisplay(WindowHandle window) const
{
    return {};
}

void WindowManager::setWindowEventCallback()
{
}

void WindowManager::removeWindowEventCallback()
{
}

void WindowManager::requestAttention(WindowHandle window)
{
}

void WindowManager::setWindowIcon(WindowHandle window, const u8* iconData, u32 width, u32 height)
{
}

void WindowManager::setWindowOpacity(WindowHandle window, f32 opacity)
{
}

f32 WindowManager::getWindowOpacity(WindowHandle window) const
{
    return {};
}
