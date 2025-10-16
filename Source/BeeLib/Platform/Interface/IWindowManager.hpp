/**
 * @File IWindowManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#pragma once

#include "./PlatformTypes.hpp"

namespace Bee
{
    // -----------------------------
    // 类型、状态定义
    // -----------------------------

    XIHE_PLATFORM_HANDLE(WindowHandle, u32);

    // 窗口状态
    enum class WindowState : u8
    {
        Hidden,
        Shown,
        Minimized,
        Maximized,
        Fullscreen,
        Restored
    };

    // TODO: 更换数学方法
    struct Vec2u
    {
        u32 x = {}, y = {};
    };

    // 创建信息
    struct WindowCreateInfo
    {
        std::string title{};
        Vec2u pos{};
        Vec2u size{};
        // Point2D position = {100, 100};
        // Size2D size = {800, 600};
        // WindowFlags flags = WindowFlags::Default;
    };

    // 原生平台窗口 Handle
    struct NativeWindowHandle
    {
        void* handle   = nullptr; // Windows: HWND, Linux: Window, macOS: NSWindow*
        void* instance = nullptr; // Windows: HINSTANCE, 其他平台可能为 nullptr
    };

    class IWindowManager
    {
    public:
        virtual ~IWindowManager() = default;

        // === 生命周期管理 ===
        virtual bool initialize() = 0;
        virtual void shutdown() = 0;
        virtual bool isInitialized() const = 0;

        // === 窗口创建与销毁 ===
        virtual WindowHandle createWindow(const WindowCreateInfo& createInfo) = 0;
        virtual void destroyWindow(WindowHandle window) = 0;
        virtual bool isWindowValid(WindowHandle window) const = 0;

        // === 窗口属性操作 ===
        virtual bool setWindowTitle(WindowHandle window, const std::string& title) = 0;
        virtual bool setWindowPosition(WindowHandle window, Vec2u pos) = 0;
        virtual bool setWindowSize(WindowHandle window, Vec2u size) = 0;

        virtual std::string GetWindowTitle(WindowHandle window) const = 0;
        virtual Vec2u GetWindowPosition(WindowHandle window) const = 0;
        virtual Vec2u GetWindowSize(WindowHandle window) const = 0;

        // === 窗口状态控制 ===
        virtual bool showWindow(WindowHandle window) = 0;
        virtual bool hideWindow(WindowHandle window) = 0;

        virtual bool minimizeWindow(WindowHandle window) = 0;
        virtual bool maximizeWindow(WindowHandle window) = 0;

        virtual bool restoreWindow(WindowHandle window) = 0;
        virtual bool setWindowFullscreen(WindowHandle window, bool fullscreen) = 0;

        virtual bool focusWindow(WindowHandle window) = 0;
        virtual bool isWindowFocused(WindowHandle window) const = 0;

        virtual WindowState getWindowState(WindowHandle window) const = 0;

        // === 窗口枚举与查询 ===
        virtual std::vector<WindowHandle> GetAllWindows() const = 0;
        virtual WindowHandle GetMainWindow() const = 0;
        virtual WindowHandle GetFocusedWindow() const = 0;

        // === 原生句柄访问 ===
        virtual NativeWindowHandle getNativeWindowHandle(WindowHandle window) const = 0;

        // === 显示器信息 ===
        virtual std::vector<DisplayInfo> getDisplays() const = 0;
        virtual DisplayInfo getPrimaryDisplay() const = 0;
        virtual DisplayInfo getWindowDisplay(WindowHandle window) const = 0;

        // === 事件回调注册 ===
        virtual void setWindowEventCallback() = 0;
        virtual void removeWindowEventCallback() = 0;

        // === 实用功能 ===
        virtual void requestAttention(WindowHandle window) = 0;
        virtual void setWindowIcon(WindowHandle window, const u8* iconData, u32 width, u32 height) = 0;

        virtual void setWindowOpacity(WindowHandle window, f32 opacity) = 0;
        virtual f32 getWindowOpacity(WindowHandle window) const = 0;
    };
} // namespace Bee
