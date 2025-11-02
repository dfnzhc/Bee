/**
 * @File PlatformManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Platform/Interface/PlatformTypes.hpp"
#include "Platform/Interface/IWindowManager.hpp"
#include "Platform/Interface/IInputManager.hpp"

namespace Bee
{
    class PlatformManager
    {
    public:
        struct InitConfig
        {
            bool enableWindow = true;

            // 窗口相关配置
            bool enableHighDPI = true;
            bool enableVSync   = true;
        };

    public:
        PlatformManager();
        ~PlatformManager();

        BEE_DISABLE_COPY_AND_MOVE(PlatformManager);

        // === 生命周期管理 ===
        VResult initialize(const InitConfig& config) const;
        void shutdown() const;
        bool isInitialized() const;

        void update() const;

        // === 管理器访问接口 ===
        IWindowManager* getWindowManager() const;
        IInputManager* getInputManager() const;

        // === 设置回调 ===
        using CommonEventCallback = std::function<void(const CommonEvent&)>;
        using WindowEventCallback = std::function<void(const WindowEvent&)>;
        using DropEventCallback   = std::function<void(const DropEvent&)>;
        
        void setCommonEventCallback(CommonEventCallback&& callback) const;
        void setWindowEventCallback(WindowEventCallback&& callback) const;
        void setDropEventCallback(DropEventCallback&& callback) const;

        void setKeyEventCallback(IInputManager::KeyEventCallback&& callback) const;
        void setMouseButtonCallback(IInputManager::MouseButtonCallback&& callback) const;
        void setMouseMotionCallback(IInputManager::MouseMotionCallback&& callback) const;
        void setMouseWheelCallback(IInputManager::MouseWheelCallback&& callback) const;

    private:
        class Impl;
        std::unique_ptr<Impl> _pImpl;
    };
} // namespace Bee
