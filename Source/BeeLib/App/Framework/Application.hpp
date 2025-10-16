/**
 * @File Application.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <string>
#include "Core/Base/Defines.hpp"

namespace Bee
{
    class Application
    {
    public:
        explicit Application(std::string name);
        virtual ~Application();

        BEE_DISABLE_COPY_AND_MOVE(Application);

        // ========== 生命周期管理 ==========

        bool initialize();
        void shutdown();

        int run();

        // ========== 状态查询 ==========

        bool isRunning() const;

    protected:
        // 用于子类重写
        virtual bool onInitialize();
        virtual void onShutdown();

        virtual bool onPrepareRun();
        virtual void onFinishRun();

        // 主循环控制
        void requestExit();

    private:
        std::string m_name;
        std::atomic<bool> m_running{false};
        std::atomic<bool> m_initialized{false};
    };
} // namespace Bee
