/**
 * @File Application.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/20
 * @Brief This file is part of Bee.
 */

#include "Core/Application.hpp"
#include "Core/Common.hpp"
#include "Utility/Logger.hpp"

using namespace bee;

Application::Application(const AppSettings& config) : _config(config)
{
    LogVerbose("Creating '{}'...", config.appName);
}

Application::~Application()
{
    _pWindow.reset();
}

int Application::preInit()
{
    LogVerbose("'{}' preInit", _config.appName);
    return 0;
}

int Application::init()
{
    LogVerbose("'{}' init", _config.appName);

    if (!_config.headless) {
        _pWindow = std::make_unique<Window>(_config.windowDesc, this);
    }
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

    if (_pWindow)
        _pWindow->shutdown();

    LogVerbose("'{}' terminated.", _config.appName);
}

Window* Application::window() const
{
    return _pWindow.get();
}

int Application::onInit()
{
    LogVerbose("'{}' onInit.", _config.appName);
    return 0;
}

void Application::onTick()
{
}

void Application::onShutdown()
{
    LogVerbose("'{}' onShutdown.", _config.appName);
}

void Application::onResize(u32 width, u32 height)
{
    LogVerbose("'{}' onResize({}, {}).", _config.appName, width, height);
}

bool Application::onMouseEvent(BEE_UNUSED const MouseEvent& mouseEvent)
{
    return false;
}

bool Application::onKeyEvent(BEE_UNUSED const KeyboardEvent& keyEvent)
{
    return false;
}

bool Application::onMouseKeyboardEvent(BEE_UNUSED const MouseInput& mouse, BEE_UNUSED const KeyboardInput& keyboard)
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
