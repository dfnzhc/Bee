/**
 * @File IWindowManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#pragma once

#include <vector>
#include "PlatformTypes.hpp"

namespace Bee
{
    // -----------------------------
    // 类型、状态定义
    // -----------------------------

    struct WindowErrors
    {
        static constexpr ErrorDomain kDomain = ErrorDomain::Window;

        static constexpr u16 kInitializeFailed   = 0x0001;
        static constexpr u16 kNotInitialize      = 0x0002;
        static constexpr u16 kMissFlag           = 0x0003;
        static constexpr u16 kCreateWindowFailed = 0x0004;
        static constexpr u16 kWindowNotFound     = 0x0005;
        static constexpr u16 kInternalFailure    = 0x0006;
    };

    constexpr auto MakeWindowErr(u16 errCode)
    {
        return std::unexpected{Error{WindowErrors::kDomain, errCode}};
    }

    // 窗口状态
    enum class WindowState : u8
    {
        Normal,
        Hidden,
        Shown,
        Minimized,
        Maximized,
        Fullscreen,
        Restored
    };

    // 窗口标志
    enum class WindowFlags : u32
    {
        None          = 0x00000000,
        Resizable     = 0x00000001,
        Borderless    = 0x00000002,
        Transparent   = 0x00000004,
        MouseRelative = 0x00000008,
        AlwaysOnTop   = 0x00000010,
        SkipTaskbar   = 0x00000020,
        HighDPI       = 0x00000040,

        Vulkan = 0x00001000,
        OpenGL = 0x00002000,
        METAL  = 0x00004000,

        // 一些预设
        Default = Resizable
    };

    BEE_ENUM_CLASS_OPERATORS(WindowFlags)

    // 创建信息
    struct WindowCreateInfo
    {
        std::string title{};
        int2 pos{};
        int2 size{};
        WindowFlags flags = WindowFlags::Default;
    };

    // 原生平台窗口 Handle
    struct NativeWindowHandle
    {
        #ifdef BEE_ON_WINDOWS
        HWND hwnd           = nullptr;
        HDC hdc             = nullptr;
        HINSTANCE hinstance = nullptr;
        #elif defined(BEE_ON_LINUX)
        u32 window    = 0;
        void* display = nullptr;
        #endif
    };

    class IWindowManager
    {
    public:
        virtual ~IWindowManager() = default;

        // === 生命周期管理 ===
        virtual VResult initialize() = 0;
        virtual void shutdown() = 0;
        virtual bool isInitialized() const = 0;

        // === 窗口创建与销毁 ===
        virtual Result<WindowHandle> createWindow(const WindowCreateInfo& createInfo) = 0;
        virtual void destroyWindow(WindowHandle window) = 0;
        virtual bool isWindowValid(WindowHandle window) const = 0;

        // === 窗口属性操作 ===
        virtual bool setWindowTitle(WindowHandle window, const std::string& title) = 0;
        virtual bool setWindowPosition(WindowHandle window, int2 pos) = 0;
        virtual bool setWindowSize(WindowHandle window, int2 size) = 0;

        virtual std::string GetWindowTitle(WindowHandle window) const = 0;
        virtual Result<int2> GetWindowPosition(WindowHandle window) const = 0;
        virtual Result<int2> GetWindowSize(WindowHandle window) const = 0;

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
        virtual bool setWindowIcon(WindowHandle window, const u8* iconData, u32 width, u32 height) = 0;

        virtual bool setWindowOpacity(WindowHandle window, f32 opacity) = 0;
        virtual f32 getWindowOpacity(WindowHandle window) const = 0;
    };
} // namespace Bee
