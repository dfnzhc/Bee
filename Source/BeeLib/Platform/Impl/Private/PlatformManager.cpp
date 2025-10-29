/**
 * @File PlatformManager.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#include "PlatformManager.hpp"
#include "Window/WindowManager.hpp"
#include "SDLHeader.hpp"

#include "Core/Error/Exception.hpp"

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

    VResult initialize(const InitConfig& config)
    {
        if (initialized)
        {
            return Ok();
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
            return MakeWindowErr(WindowErrors::kInitializeFailed);
        }

        ScopeGuardian sdlGuard;
        sdlGuard.onCleanup([]
        {
            SDL_Quit();
        });

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
        BEE_TRY_VOID(initializeManagers());

        sdlGuard.dismiss();
        initialized = true;
        return Ok();
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
    VResult initializeManagers()
    {
        windowManager = std::make_unique<WindowManager>();
        BEE_TRY_VOID(windowManager->initialize());

        return Ok();
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

    std::unique_ptr<IWindowManager> windowManager = nullptr;
};

PlatformManager::PlatformManager() :
    _pImpl(std::make_unique<Impl>())
{
}

PlatformManager::~PlatformManager()
{
    _pImpl->shutdown();
}

VResult PlatformManager::initialize(const InitConfig& config) const
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

IWindowManager* PlatformManager::getWindowManager() const
{
    return _pImpl->windowManager.get();
}
