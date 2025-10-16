/**
 * @File PlatformManager.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#include "PlatformManager.hpp"
#include "Window/WindowManager.hpp"

#include "Core/Error/Exception.hpp"

BEE_PUSH_WARNING
BEE_CLANG_DISABLE_WARNING("-Wreserved-macro-identifier")
BEE_CLANG_DISABLE_WARNING("-Wdocumentation-unknown-command")
#include <SDL3/SDL.h>
BEE_POP_WARNING

using namespace Bee;

class PlatformManager::Impl
{
public:
    Impl() :
        initialized(false)
    {
    }

    ~Impl()
    {
        if (initialized)
        {
            shutdown();
        }
    }

    bool initialize(const InitConfig& config)
    {
        if (initialized)
        {
            return true;
        }

        initConfig = config;

        // 初始化SDL3核心
        u32 sdlFlags = 0;
        if (config.enableWindow)
        {
            sdlFlags |= SDL_INIT_VIDEO | SDL_INIT_EVENTS;
        }

        if (!SDL_Init(sdlFlags))
        {
            BEE_ERROR("SDL3 初始化失败: {}.", SDL_GetError());
            return false;
        }

        // 设置SDL3错误回调
        SDL_SetLogOutputFunction([](void* /* userdata */, int /* category */, SDL_LogPriority priority, const char* message)
        {
            // clang-format off
            BEE_PUSH_WARNING
            BEE_CLANG_DISABLE_WARNING("-Wswitch-enum")
            switch (priority)
            {
                case SDL_LOG_PRIORITY_WARN: BEE_WARN("[SDL]: {}", message); break;
                case SDL_LOG_PRIORITY_ERROR: BEE_ERROR("[SDL]: {}", message); break;
                case SDL_LOG_PRIORITY_CRITICAL: BEE_FATAL("[SDL]: {}", message); break;
                default: break;
            }
            BEE_POP_WARNING
            // clang-format on
        }, this);

        // 初始化各个管理器
        if (!initializeManagers())
        {
            SDL_Quit();
            return false;
        }

        initialized = true;
        return true;
    }

    void shutdown()
    {
        if (!initialized)
        {
            return;
        }

        shutdownManagers();
        SDL_Quit();

        initialized = false;
    }

    void update()
    {
        if (!initialized)
        {
            return;
        }
    }

private:
    bool initializeManagers()
    {
        windowManager = std::make_unique<WindowManager>();
        if (!windowManager->initialize())
        {
            BEE_ERROR("窗口管理器初始化失败.");
            return false;
        }

        return true;
    }

    void shutdownManagers()
    {
        if (windowManager)
        {
            windowManager->shutdown();
            windowManager.reset();
        }
    }

public:
    InitConfig initConfig;

    bool initialized;

    // 管理器实例
    std::unique_ptr<IWindowManager> windowManager;
};

PlatformManager::PlatformManager() :
    _pImpl(std::make_unique<Impl>())
{
}

PlatformManager::~PlatformManager()
{
    _pImpl->shutdown();
}

bool PlatformManager::initialize(const InitConfig& config)
{
    return _pImpl->initialize(config);
}

void PlatformManager::shutdown()
{
    return _pImpl->shutdown();
}

bool PlatformManager::isInitialized() const
{
    return _pImpl->initialized;
}

void PlatformManager::update()
{
    return _pImpl->update();
}
