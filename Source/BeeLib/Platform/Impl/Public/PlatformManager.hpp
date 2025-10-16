/**
 * @File PlatformManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Platform/Interface/PlatformTypes.hpp"
#include "Platform/Interface/IWindowManager.hpp"

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
        bool initialize(const InitConfig& config);
        void shutdown();
        bool isInitialized() const;

        void update();

        // === 管理器访问接口 ===

    private:
        class Impl;
        std::unique_ptr<Impl> _pImpl;
    };
} // namespace Bee
