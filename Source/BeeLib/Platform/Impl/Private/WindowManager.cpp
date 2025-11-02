/**
 * @File WindowManager.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/18
 * @Brief This file is part of Bee.
 */

#include "WindowManager.hpp"
#include "SDLHeader.hpp"

using namespace Bee;

VResult WindowManager::initialize()
{
    if (_initialized)
    {
        return Ok();
    }

    SDL_InitFlags initFlags = SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if ((initFlags & SDL_INIT_VIDEO) == 0)
    {
        BEE_ERROR("SDL 初始化时没有设置 'SDL_INIT_VIDEO' 标志.");
        return MakeWindowErr(WindowErrors::kMissFlag);
    }

    _initialized = true;
    return Ok();
}

void WindowManager::shutdown()
{
    if (!_initialized)
    {
        return;
    }

    cleanupAllWindows();

    _initialized   = false;
    _mainWindow    = WindowHandle{0};
    _focusedWindow = WindowHandle{0};
    _nextWindowId  = 1;
}

bool WindowManager::isInitialized() const
{
    return _initialized;
}

Result<WindowHandle> WindowManager::createWindow(const WindowCreateInfo& createInfo)
{
    if (!_initialized)
    {
        BEE_ERROR("WindowManager 没有被正确初始化.");
        return MakeWindowErr(WindowErrors::kNotInitialize);
    }

    u32 sdlFlags          = convertWindowFlags(createInfo.flags);
    SDL_Window* sdlWindow = SDL_CreateWindow(createInfo.title.c_str(), createInfo.size.x, createInfo.size.y, sdlFlags);
    if (!sdlWindow)
    {
        BEE_ERROR("创建 SDL 窗口失败: {}", SDL_GetError());
        return MakeWindowErr(WindowErrors::kCreateWindowFailed);
    }

    if (createInfo.pos.x != 0 && createInfo.pos.y != 0)
    {
        SDL_SetWindowPosition(sdlWindow, createInfo.pos.x, createInfo.pos.y);
    }

    u32 windowId = generateWindowId();
    auto handle  = WindowHandle{windowId};

    auto windowData        = std::make_unique<WindowData>();
    windowData->sdlWindow  = sdlWindow;
    windowData->createInfo = createInfo;
    windowData->handle     = handle;
    windowData->isValid    = true;

    _windows[windowId]        = std::move(windowData);
    _sdlWindowToId[sdlWindow] = windowId;

    // 设置主窗口
    if (_mainWindow.handle == 0)
    {
        _mainWindow = handle;
    }

    return handle;
}

void WindowManager::destroyWindow(WindowHandle window)
{
    if (!_initialized)
    {
        return;
    }

    WindowData* windowData = BEE_TRY_OR_RETURN_VOID(getWindowData(window));
    cleanupWindow(windowData);

    _sdlWindowToId.erase(windowData->sdlWindow);
    _windows.erase(window.handle);

    // 更新主窗口和焦点窗口
    if (_mainWindow.handle == window.handle)
    {
        _mainWindow = _windows.empty() ? WindowHandle{0} : WindowHandle{_windows.begin()->first};
    }
    if (_focusedWindow.handle == window.handle)
    {
        _focusedWindow = WindowHandle{0};
    }

    BEE_INFO("销毁窗口('{}')成功.", window.handle);
}

bool WindowManager::isWindowValid(WindowHandle window) const
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));
    return windowData && windowData->isValid;
}

bool WindowManager::setWindowTitle(WindowHandle window, const std::string& title)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    if (!SDL_SetWindowTitle(windowData->sdlWindow, title.c_str()))
    {
        BEE_WARN("设置窗口('{}')标题失败: {}.", window.handle, SDL_GetError());
        return false;
    }

    windowData->createInfo.title = title;
    return true;
}

bool WindowManager::setWindowPosition(WindowHandle window, int2 pos)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    if (!SDL_SetWindowPosition(windowData->sdlWindow, pos.x, pos.y))
    {
        BEE_WARN("设置窗口('{}')位置失败: {}.", window.handle, SDL_GetError());
        return false;
    }
    return true;
}

bool WindowManager::setWindowSize(WindowHandle window, int2 size)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    if (!SDL_SetWindowSize(windowData->sdlWindow, size.x, size.y))
    {
        BEE_WARN("设置窗口('{}')尺寸失败: {}.", window.handle, SDL_GetError());
        return false;
    }
    return true;
}

std::string WindowManager::GetWindowTitle(WindowHandle window) const
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    const char* title = SDL_GetWindowTitle(windowData->sdlWindow);
    return title ? title : "";
}

Result<int2> WindowManager::GetWindowPosition(WindowHandle window) const
{
    WindowData* windowData = BEE_TRY(getWindowData(window));

    int x, y;
    if (!SDL_GetWindowPosition(windowData->sdlWindow, &x, &y))
    {
        BEE_WARN("获取窗口('{}')位置失败: {}.", window.handle, SDL_GetError());
        return MakeWindowErr(WindowErrors::kInternalFailure);
    }
    return int2{x, y};
}

Result<int2> WindowManager::GetWindowSize(WindowHandle window) const
{
    WindowData* windowData = BEE_TRY(getWindowData(window));

    int width, height;
    if (!SDL_GetWindowSize(windowData->sdlWindow, &width, &height))
    {
        BEE_WARN("获取窗口('{}')尺寸失败: {}.", window.handle, SDL_GetError());
        return MakeWindowErr(WindowErrors::kInternalFailure);
    }
    return int2{width, height};
}

bool WindowManager::showWindow(WindowHandle window)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    if (!SDL_ShowWindow(windowData->sdlWindow))
    {
        BEE_WARN("显示窗口('{}')失败: {}.", window.handle, SDL_GetError());
        return false;
    }
    return true;
}

bool WindowManager::hideWindow(WindowHandle window)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    if (!SDL_HideWindow(windowData->sdlWindow))
    {
        BEE_WARN("隐藏窗口('{}')失败: {}.", window.handle, SDL_GetError());
        return false;
    }
    return true;
}

bool WindowManager::minimizeWindow(WindowHandle window)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    if (!SDL_MinimizeWindow(windowData->sdlWindow))
    {
        BEE_WARN("最小化窗口('{}')失败: {}.", window.handle, SDL_GetError());
        return {};
    }
    return true;
}

bool WindowManager::maximizeWindow(WindowHandle window)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    if (!SDL_MaximizeWindow(windowData->sdlWindow))
    {
        BEE_WARN("最大化窗口('{}')失败: {}.", window.handle, SDL_GetError());
        return {};
    }
    return true;
}

bool WindowManager::restoreWindow(WindowHandle window)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    if (!SDL_RestoreWindow(windowData->sdlWindow))
    {
        BEE_WARN("恢复窗口('{}')失败: {}.", window.handle, SDL_GetError());
        return {};
    }

    return true;
}

bool WindowManager::setWindowFullscreen(WindowHandle window, bool fullscreen)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    u32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
    if (!SDL_SetWindowFullscreen(windowData->sdlWindow, flags))
    {
        BEE_WARN("设置窗口('{}')全屏失败: {}.", window.handle, SDL_GetError());
        return {};
    }
    return true;
}

bool WindowManager::focusWindow(WindowHandle window)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    if (!SDL_RaiseWindow(windowData->sdlWindow))
    {
        BEE_WARN("聚焦窗口('{}')失败: {}.", window.handle, SDL_GetError());
        return {};
    }
    _focusedWindow = window;
    return true;
}

bool WindowManager::isWindowFocused(WindowHandle window) const
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    SDL_WindowFlags flags = SDL_GetWindowFlags(windowData->sdlWindow);
    return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

WindowState WindowManager::getWindowState(WindowHandle window) const
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    SDL_WindowFlags flags = SDL_GetWindowFlags(windowData->sdlWindow);
    return convertSDLWindowState(flags);
}

std::vector<WindowHandle> WindowManager::GetAllWindows() const
{
    std::vector<WindowHandle> windows;
    windows.reserve(_windows.size());

    for (const auto& pair : _windows)
    {
        if (pair.second && pair.second->isValid)
        {
            windows.emplace_back(pair.first);
        }
    }

    return windows;
}

WindowHandle WindowManager::GetMainWindow() const
{
    return _mainWindow;
}

WindowHandle WindowManager::GetFocusedWindow() const
{
    return _focusedWindow;
}

NativeWindowHandle WindowManager::getNativeWindowHandle(WindowHandle window) const
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    // 获取SDL窗口的原生句柄
    SDL_PropertiesID props = SDL_GetWindowProperties(windowData->sdlWindow);

    // clang-format off
    NativeWindowHandle nativeHandle;
    #ifdef BEE_ON_WINDOWS
    nativeHandle.hwnd      = static_cast<HWND>(SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    nativeHandle.hdc       = static_cast<HDC>(SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HDC_POINTER, nullptr));
    nativeHandle.hinstance = static_cast<HINSTANCE>(SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr));
    #elif defined(BEE_ON_LINUX)
    nativeHandle.window  = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    nativeHandle.display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    #endif
    // clang-format on

    return nativeHandle;
}

std::vector<DisplayInfo> WindowManager::getDisplays() const
{
    std::vector<DisplayInfo> displays;

    if (!_initialized)
    {
        return displays;
    }

    int displayCount          = 0;
    SDL_DisplayID* displayIDs = SDL_GetDisplays(&displayCount);
    if (!(displayIDs && displayCount > 0))
    {
        return displays;
    }

    displays.reserve(static_cast<Size>(displayCount));
    for (int i = 0; i < displayCount; ++i)
    {
        displays.push_back(convertSDLDisplay(i));
    }

    SDL_free(displayIDs);
    return displays;
}

DisplayInfo WindowManager::getPrimaryDisplay() const
{
    if (!_initialized)
    {
        return DisplayInfo{};
    }

    constexpr int primaryDisplayId = 0;
    return convertSDLDisplay(primaryDisplayId);
}

DisplayInfo WindowManager::getWindowDisplay(WindowHandle window) const
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    SDL_DisplayID displayID = SDL_GetDisplayForWindow(windowData->sdlWindow);
    if (displayID == 0)
    {
        BEE_WARN("获取窗口('{}')显示器失败: {}.", window.handle, SDL_GetError());
        return {};
    }

    // 找到对应的显示器索引
    int displayCount          = 0;
    SDL_DisplayID* displayIDs = SDL_GetDisplays(&displayCount);
    if (!displayIDs)
    {
        BEE_WARN("获取显示器失败: {}.", SDL_GetError());
        return {};
    }

    int displayIndex = -1;
    for (int i = 0; i < displayCount; ++i)
    {
        BEE_PUSH_WARNING
        BEE_CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
        if (displayIDs[i] == displayID)
        {
            displayIndex = i;
            break;
        }
        BEE_POP_WARNING
    }

    SDL_free(displayIDs);
    if (displayIndex == -1)
    {
        BEE_WARN("没有获取到有效的显示器.");
        return {};
    }

    return convertSDLDisplay(displayIndex);
}

void WindowManager::setWindowEventCallback()
{
    // TODO:
}

void WindowManager::removeWindowEventCallback()
{
    // TODO:
}

void WindowManager::requestAttention(WindowHandle window)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_VOID(getWindowData(window));

    SDL_FlashWindow(windowData->sdlWindow, SDL_FLASH_UNTIL_FOCUSED);
}

bool WindowManager::setWindowIcon(WindowHandle window, const u8* iconData, u32 width, u32 height)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    // 创建SDL表面
    SDL_Surface* iconSurface = SDL_CreateSurfaceFrom(
            static_cast<int>(width), static_cast<int>(height),
            SDL_PIXELFORMAT_RGBA32,
            const_cast<uint8_t*>(iconData),
            static_cast<int>(width * 4)
            );

    if (!iconSurface)
    {
        BEE_WARN("创建 SDL Surface 失败: {}.", SDL_GetError());
        return {};
    }
    ScopeGuardian guardian;
    guardian.onCleanup([&]
    {
        SDL_DestroySurface(iconSurface);
    });

    if (!SDL_SetWindowIcon(windowData->sdlWindow, iconSurface))
    {
        BEE_WARN("设置窗口('{}')图标失败: {}.", window.handle, SDL_GetError());
        return false;
    }
    return true;
}

bool WindowManager::setWindowOpacity(WindowHandle window, f32 opacity)
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (!SDL_SetWindowOpacity(windowData->sdlWindow, opacity))
    {
        BEE_WARN("设置窗口('{}')透明度失败: {}.", window.handle, SDL_GetError());
        return false;
    }
    return true;
}

f32 WindowManager::getWindowOpacity(WindowHandle window) const
{
    WindowData* windowData = BEE_TRY_OR_RETURN_DEFAULT(getWindowData(window));

    const f32 opacity = SDL_GetWindowOpacity(windowData->sdlWindow);
    if (opacity < 0.0f || opacity > 1.0f)
    {
        BEE_WARN("获取窗口('{}')透明度失败: {}.", window.handle, SDL_GetError());
        return -1.0f;
    }

    return opacity;
}

Result<WindowManager::WindowData*> WindowManager::getWindowData(WindowHandle window) const
{
    if (!_windows.contains(window.handle))
    {
        BEE_ERROR("没有找到该窗口: '{}'.", window.handle);
        return MakeWindowErr(WindowErrors::kWindowNotFound);
    }

    return _windows.at(window.handle).get();
}

Result<WindowManager::WindowData*> WindowManager::getWindowDataBySDL(SDL_Window* sdlWindow) const
{
    auto it = _sdlWindowToId.find(sdlWindow);
    if (it == _sdlWindowToId.end())
    {
        return nullptr;
    }
    return getWindowData(WindowHandle{it->second});
}

u32 WindowManager::generateWindowId()
{
    return _nextWindowId++;
}

u32 WindowManager::convertWindowFlags(WindowFlags flags) const
{
    u32 sdlFlags = 0;

    if (IsSet(flags, WindowFlags::Resizable))
    {
        sdlFlags |= SDL_WINDOW_RESIZABLE;
    }
    if (IsSet(flags, WindowFlags::Borderless))
    {
        sdlFlags |= SDL_WINDOW_BORDERLESS;
    }
    if (IsSet(flags, WindowFlags::Transparent))
    {
        sdlFlags |= SDL_WINDOW_TRANSPARENT;
    }
    if (IsSet(flags, WindowFlags::MouseRelative))
    {
        sdlFlags |= SDL_WINDOW_MOUSE_RELATIVE_MODE;
    }
    if (IsSet(flags, WindowFlags::AlwaysOnTop))
    {
        sdlFlags |= SDL_WINDOW_ALWAYS_ON_TOP;
    }
    if (IsSet(flags, WindowFlags::SkipTaskbar))
    {
        sdlFlags |= SDL_WINDOW_UTILITY;
    }
    if (IsSet(flags, WindowFlags::HighDPI))
    {
        sdlFlags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    }
    if (IsSet(flags, WindowFlags::Vulkan))
    {
        sdlFlags |= SDL_WINDOW_VULKAN;
    }
    if (IsSet(flags, WindowFlags::OpenGL))
    {
        sdlFlags |= SDL_WINDOW_OPENGL;
    }
    if (IsSet(flags, WindowFlags::METAL))
    {
        sdlFlags |= SDL_WINDOW_METAL;
    }

    return sdlFlags;
}

WindowState WindowManager::convertSDLWindowState(u64 sdlFlags) const
{
    if (sdlFlags & SDL_WINDOW_MINIMIZED)
    {
        return WindowState::Minimized;
    }
    if (sdlFlags & SDL_WINDOW_MAXIMIZED)
    {
        return WindowState::Maximized;
    }
    if (sdlFlags & SDL_WINDOW_FULLSCREEN)
    {
        return WindowState::Fullscreen;
    }
    if (sdlFlags & SDL_WINDOW_HIDDEN)
    {
        return WindowState::Hidden;
    }

    return WindowState::Normal;
}

DisplayInfo WindowManager::convertSDLDisplay(int displayIndex) const
{
    DisplayInfo info{};
    info.index = displayIndex;

    SDL_DisplayID* displayIDs = nullptr;
    int displayCount          = 0;
    displayIDs                = SDL_GetDisplays(&displayCount);

    if (!displayIDs || displayIndex >= displayCount)
    {
        return info;
    }

    BEE_PUSH_WARNING
    BEE_CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
    SDL_DisplayID displayID = displayIDs[displayIndex];
    BEE_POP_WARNING

    const char* name = SDL_GetDisplayName(displayID);
    info.name        = name ? name : "";

    SDL_Rect bounds;
    if (SDL_GetDisplayBounds(displayID, &bounds))
    {
        info.posX   = bounds.x;
        info.posY   = bounds.y;
        info.width  = static_cast<uint32_t>(bounds.w);
        info.height = static_cast<uint32_t>(bounds.h);
    }

    // 工作区域 SDL_GetDisplayUsableBounds

    info.dpi = SDL_GetDisplayContentScale(displayID);

    // 获取刷新率
    if (const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID))
    {
        info.refreshRate = mode->refresh_rate;
    }

    info.isPrimary = (displayIndex == 0);

    SDL_free(displayIDs);
    return info;
}

void WindowManager::cleanupWindow(WindowData* windowData)
{
    if (windowData && windowData->sdlWindow)
    {
        SDL_DestroyWindow(windowData->sdlWindow);
        windowData->sdlWindow = nullptr;
        windowData->isValid   = false;
    }
}

void WindowManager::cleanupAllWindows()
{
    for (auto& pair : _windows)
    {
        cleanupWindow(pair.second.get());
    }
    _windows.clear();
    _sdlWindowToId.clear();
}
