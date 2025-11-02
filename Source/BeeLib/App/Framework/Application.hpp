/**
 * @File Application.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <string>
#include <memory>
#include "Core/Base/Defines.hpp"
#include "Platform/Interface/PlatformTypes.hpp"

namespace Bee
{
    class Application
    {
    public:
        explicit Application(std::string name);
        virtual ~Application();

        BEE_DISABLE_COPY_AND_MOVE(Application);

        // === 生命周期管理 ===
        bool initialize();
        void shutdown();

        int run();

        // === 状态查询 ===
        bool isRunning() const;

    protected:
        // === 用于子类重写 ===
        virtual bool onInitialize();
        virtual void onShutdown();

        virtual bool onPrepareRun();
        virtual void onFinishRun();

        virtual void drawFrame();

        virtual void onHandleCommonEvent(const CommonEvent& /*event*/);
        virtual void onHandleWindowEvent(const WindowEvent& /*event*/);
        virtual void onHandleDropEvent(const DropEvent& /*event*/);

        virtual void onHandleKeyboardEvent(const KeyboardEvent& /*event*/);
        virtual void onHandleMouseButtonEvent(const MouseButtonEvent& /*event*/);
        virtual void onHandleMouseMotionEvent(const MouseMotionEvent& /*event*/);
        virtual void onHandleMouseWheelEvent(const MouseWheelEvent& /*event*/);

        // === 主循环控制 ===
        void requestExit();

    private:
        void update() const;

        void handleCommonEvent(const CommonEvent& event);
        void handleWindowEvent(const WindowEvent& event);
        void handleDropEvent(const DropEvent& event);

        void handleKeyboardEvent(const KeyboardEvent& event);
        void handleMouseButtonEvent(const MouseButtonEvent& event);
        void handleMouseMotionEvent(const MouseMotionEvent& event);
        void handleMouseWheelEvent(const MouseWheelEvent& event);

    protected:
        std::unique_ptr<class PlatformManager> _pPlatform;

    private:
        std::string m_name;
        std::atomic<bool> m_running{false};
        std::atomic<bool> m_initialized{false};
    };
} // namespace Bee
