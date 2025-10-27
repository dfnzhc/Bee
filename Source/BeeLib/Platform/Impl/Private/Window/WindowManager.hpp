/**
 * @File WindowManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

#include "Platform/Interface/IWindowManager.hpp"

struct SDL_Window;
union SDL_Event;

// TODO: 返回结果中增加可能的错误信息

namespace Bee
{
    class WindowManager : public IWindowManager
    {
    public:
        WindowManager()           = default;
        ~WindowManager() override = default;

        BEE_DISABLE_COPY_AND_MOVE(WindowManager);

        // === 生命周期管理 ===
        bool initialize() override;
        void shutdown() override;
        bool isInitialized() const override;

        // === 窗口创建与销毁 ===
        WindowHandle createWindow(const WindowCreateInfo& createInfo) override;
        void destroyWindow(WindowHandle window) override;
        bool isWindowValid(WindowHandle window) const override;

        // === 窗口属性操作 ===
        bool setWindowTitle(WindowHandle window, const std::string& title) override;
        bool setWindowPosition(WindowHandle window, int2 pos) override;
        bool setWindowSize(WindowHandle window, int2 size) override;

        std::string GetWindowTitle(WindowHandle window) const override;
        int2 GetWindowPosition(WindowHandle window) const override;
        int2 GetWindowSize(WindowHandle window) const override;

        // === 窗口状态控制 ===
        bool showWindow(WindowHandle window) override;
        bool hideWindow(WindowHandle window) override;

        bool minimizeWindow(WindowHandle window) override;
        bool maximizeWindow(WindowHandle window) override;

        bool restoreWindow(WindowHandle window) override;
        bool setWindowFullscreen(WindowHandle window, bool fullscreen) override;

        bool focusWindow(WindowHandle window) override;
        bool isWindowFocused(WindowHandle window) const override;

        WindowState getWindowState(WindowHandle window) const override;

        // === 窗口枚举与查询 ===
        std::vector<WindowHandle> GetAllWindows() const override;
        WindowHandle GetMainWindow() const override;
        WindowHandle GetFocusedWindow() const override;

        // === 原生句柄访问 ===
        NativeWindowHandle getNativeWindowHandle(WindowHandle window) const override;

        // === 显示器信息 ===
        std::vector<DisplayInfo> getDisplays() const override;
        DisplayInfo getPrimaryDisplay() const override;
        DisplayInfo getWindowDisplay(WindowHandle window) const override;

        // === 事件回调注册 ===
        void setWindowEventCallback() override;
        void removeWindowEventCallback() override;

        // === 实用功能 ===
        void requestAttention(WindowHandle window) override;
        void setWindowIcon(WindowHandle window, const u8* iconData, u32 width, u32 height) override;

        void setWindowOpacity(WindowHandle window, f32 opacity) override;
        f32 getWindowOpacity(WindowHandle window) const override;

    private:
        struct WindowData
        {
            SDL_Window* sdlWindow{nullptr};
            WindowCreateInfo createInfo{};
            WindowHandle handle{0};
            bool isValid{false};
        };

        // === 内部辅助方法 ===
        WindowData* getWindowData(WindowHandle window) const;
        WindowData* getWindowDataBySDL(SDL_Window* sdlWindow) const;

        u32 generateWindowId();
        u32 convertWindowFlags(WindowFlags flags) const;

        WindowState convertSDLWindowState(u64 sdlFlags) const;
        DisplayInfo convertSDLDisplay(int displayIndex) const;

        void cleanupWindow(WindowData* windowData);
        void cleanupAllWindows();

    private:
        // --- 窗口管理 ---
        std::unordered_map<u32, std::unique_ptr<WindowData>> _windows;
        std::unordered_map<SDL_Window*, u32> _sdlWindowToId;

        // --- 内部状态 ---
        WindowHandle _focusedWindow{0};
        WindowHandle _mainWindow{0};

        u32 _nextWindowId{1};
        bool _initialized{false};
    };
} // namespace Bee
