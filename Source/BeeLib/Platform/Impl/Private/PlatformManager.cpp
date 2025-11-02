/**
 * @File PlatformManager.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#include "PlatformManager.hpp"

#include "EventPump.hpp"
#include "WindowManager.hpp"
#include "InputManager.hpp"

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
        if (!Logger::Instance().isInitialized())
        {
            std::cerr << "[Bee] Logger 初始化失败\n";
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

        eventPump = std::make_unique<EventPump>();
        eventPump->setEventCallback([this](PlatformEvent&& event)
        {
            dispatchEvent(std::forward<PlatformEvent>(event));
        });

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
        Logger::Instance().shutdown();
    }

    void update() const
    {
        if (!initialized)
        {
            return;
        }

        eventPump->pumpEvents();
        inputManager->updateFrame();
    }

    void setCommonEventCallback(CommonEventCallback&& callback)
    {
        _commonCallback = std::move(callback);
    }

    void setWindowEventCallback(WindowEventCallback&& callback)
    {
        _windowCallback = std::move(callback);
    }

    void setDropEventCallback(DropEventCallback&& callback)
    {
        _dropCallback = std::move(callback);
    }

private:
    VResult initializeManagers()
    {
        windowManager = std::make_unique<WindowManager>();
        BEE_TRY_VOID(windowManager->initialize());

        inputManager = std::make_unique<InputManager>();
        BEE_TRY_VOID(inputManager->initialize());

        return Ok();
    }

    void shutdownManagers()
    {
        if (inputManager)
        {
            inputManager->shutdown();
            inputManager.reset();
        }
        if (windowManager)
        {
            windowManager->shutdown();
            windowManager.reset();
        }
    }

    void dispatchEvent(PlatformEvent&& event) const
    {
        if (std::holds_alternative<KeyboardEvent>(event))
        {
            inputManager->processInputEvent(std::get<KeyboardEvent>(event));
        }
        else if (std::holds_alternative<MouseButtonEvent>(event))
        {
            inputManager->processInputEvent(std::get<MouseButtonEvent>(event));
        }
        else if (std::holds_alternative<MouseMotionEvent>(event))
        {
            inputManager->processInputEvent(std::get<MouseMotionEvent>(event));
        }
        else if (std::holds_alternative<MouseWheelEvent>(event))
        {
            inputManager->processInputEvent(std::get<MouseWheelEvent>(event));
        }

        try
        {
            if (std::holds_alternative<CommonEvent>(event))
            {
                if (_commonCallback)
                {
                    _commonCallback(std::get<CommonEvent>(event));
                }
            }
            else if (std::holds_alternative<WindowEvent>(event))
            {
                if (_windowCallback)
                {
                    _windowCallback(std::get<WindowEvent>(event));
                }
            }
            else if (std::holds_alternative<DropEvent>(event))
            {
                if (_dropCallback)
                {
                    _dropCallback(std::get<DropEvent>(event));
                }
            }
        }
        catch (const std::exception& e)
        {
            BEE_ERROR("分发事件有错误: {}.", e.what());
        }
    }

public:
    InitConfig initConfig;

    std::unique_ptr<IWindowManager> windowManager = nullptr;
    std::unique_ptr<IInputManager> inputManager   = nullptr;
    std::unique_ptr<EventPump> eventPump          = nullptr;

    CommonEventCallback _commonCallback = {};
    WindowEventCallback _windowCallback = {};
    DropEventCallback _dropCallback     = {};

    bool initialized = false;
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

void PlatformManager::shutdown() const
{
    return _pImpl->shutdown();
}

bool PlatformManager::isInitialized() const
{
    return _pImpl->initialized;
}

void PlatformManager::update() const
{
    return _pImpl->update();
}

IWindowManager* PlatformManager::getWindowManager() const
{
    return _pImpl->windowManager.get();
}

IInputManager* PlatformManager::getInputManager() const
{
    return _pImpl->inputManager.get();
}

void PlatformManager::setCommonEventCallback(CommonEventCallback&& callback) const
{
    _pImpl->setCommonEventCallback(std::forward<CommonEventCallback>(callback));
}

void PlatformManager::setWindowEventCallback(WindowEventCallback&& callback) const
{
    _pImpl->setWindowEventCallback(std::forward<WindowEventCallback>(callback));
}

void PlatformManager::setDropEventCallback(DropEventCallback&& callback) const
{
    _pImpl->setDropEventCallback(std::forward<DropEventCallback>(callback));
}

void PlatformManager::setKeyEventCallback(IInputManager::KeyEventCallback&& callback) const
{
    _pImpl->inputManager->setKeyEventCallback(std::forward<IInputManager::KeyEventCallback>(callback));
}

void PlatformManager::setMouseButtonCallback(IInputManager::MouseButtonCallback&& callback) const
{
    _pImpl->inputManager->setMouseButtonCallback(std::forward<IInputManager::MouseButtonCallback>(callback));
}

void PlatformManager::setMouseMotionCallback(IInputManager::MouseMotionCallback&& callback) const
{
    _pImpl->inputManager->setMouseMotionCallback(std::forward<IInputManager::MouseMotionCallback>(callback));
}

void PlatformManager::setMouseWheelCallback(IInputManager::MouseWheelCallback&& callback) const
{
    _pImpl->inputManager->setMouseWheelCallback(std::forward<IInputManager::MouseWheelCallback>(callback));
}
