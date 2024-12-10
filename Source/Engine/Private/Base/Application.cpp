/**
 * @File Application.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/20
 * @Brief This file is part of Bee.
 */

#include "Base/Application.hpp"
#include <Utility/Logger.hpp>
#include "Render/RenderDevice.hpp"

using namespace bee;

Application::Application(const AppConfig& config) : _config(config)
{
    LogInfo("{} 正在创建...", config.appName);

    if (!config.headless) {
        _pWindow = std::make_unique<Window>(config.windowDesc, this);
    }

    LogInfo("{} 创建完成.", config.appName);
}

Application::~Application()
{
    _pWindow.reset();
}

int Application::preInit()
{
    return 0;
}

int Application::init()
{
    _pRenderDevice = std::make_unique<RenderDevice>();
    auto res       = _pRenderDevice->create(createDevices());
    BEE_ASSERT(res == Error::Ok, "渲染设备创建失败");

    _inputManager.subscribe([this](const MouseInput& mouse, const KeyboardInput& keyboard) { handleMouseKeyboardEvent(mouse, keyboard); });

    return onInit();
}

void Application::tick()
{
    if (_shouldTerminate)
        return;

    _pWindow->pollForEvents();

    onTick();

    _inputManager.tick();
}

void Application::shutdown()
{
    onShutdown();

    _pRenderDevice->destroy();

    if (_pWindow)
        _pWindow->shutdown();

    LogInfo("{} 已关闭.", _config.appName);
}

Window* Application::window() const
{
    return _pWindow.get();
}

int Application::onInit()
{
    return 0;
}

void Application::onTick()
{
}

void Application::onShutdown()
{
}

void Application::onResize(u32 width, u32 height)
{
}

bool Application::onMouseEvent(const MouseEvent& mouseEvent)
{
    return false;
}

bool Application::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return false;
}

bool Application::onMouseKeyboardEvent(const MouseInput& mouse, const KeyboardInput& keyboard)
{
    return false;
}

void Application::handleWindowSizeChange(u32 width, u32 height)
{
    onResize(width, height);
}

void Application::handleKeyboardEvent(const KeyboardEvent& keyEvent)
{
    _inputManager.onKeyboardEvent(keyEvent);
}

void Application::handleMouseEvent(const MouseEvent& mouseEvent)
{
    _inputManager.onMouseEvent(mouseEvent);
}

void Application::handleMouseKeyboardEvent(const MouseInput& mouse, const KeyboardInput& keyboard)
{
    if (onMouseKeyboardEvent(mouse, keyboard))
        return;

    if (keyboard.isKeyPressed(KeyboardEvent::Key::Escape)) {
        _shouldTerminate = true;
    }
}

bool Application::shouldTerminate() const
{
    return _shouldTerminate;
}

RenderDeviceConfig Application::createDevices() const
{
    RenderDeviceConfig rdc{};
    rdc.deviceType = RenderDeviceType::Vulkan;
    rdc.headless   = _config.headless;

    return rdc;
}
