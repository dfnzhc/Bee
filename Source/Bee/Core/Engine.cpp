/**
 * @File Engine.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */

#include "Core/Engine.hpp"

#include "Property.hpp"
#include "Core/Window.hpp"

#include "Core/UI/Gui.hpp"
#include "Events/EventManager.hpp"
#include "Events/InputEvent.hpp"
#include "Events/WindowEvent.hpp"

using namespace bee;

Engine::Engine()
{
}

Engine::~Engine()
{
    _shutdown();
}

int Engine::execute()
{
    if (!_initialize()) {
        LogError("Engine initialized failed.");
        return EXIT_FAILURE;
    }

    while (Property::IsEngineRunning()) {
        _pWindow->pollForEvents();

        _update();

        EventManager::Instance().Process();
    }

    _shutdown();

    return EXIT_SUCCESS;
}

void Engine::_registerInputEvent()
{
    const auto& em = EventManager::Instance();

    // clang-format off
    em.Register(EventType::keyboard, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::keyboard); _onKeyboardEvent(event); });
    em.Register(EventType::Mouse, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::Mouse); _onMouseEvent(event); });
    em.Register(EventType::WindowResize, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::WindowResize); _onWindowSizeChanged(event); });
    // clang-format on
}

void Engine::_onWindowSizeChanged(const EventPtr& event)
{
    const auto* mouseEvent = static_cast<const WindowResizeEvent*>(event.get());

    _pGui->onWindowResize(mouseEvent->extent());
}

void Engine::_onKeyboardEvent(const EventPtr& event)
{
    const auto* keyEvent = static_cast<const KeyboardEvent*>(event.get());

    const auto key = keyEvent->key();

    if (key == Keys::Escape)
        Property::SetEngineRunning(false);
}

void Engine::_onMouseEvent(const EventPtr& event)
{
}

bool Engine::_initialize()
{
    _initializeWindow();
    SimpleCheckAndReturn(_pWindow);

    _initializeGui();
    SimpleCheckAndReturn(_pGui);

    _registerInputEvent();

    Property::SetEngineRunning(true);

    return true;
}

void Engine::_shutdown()
{
    _shutdownWindow();

    _shutdownGui();
}

void Engine::_initializeWindow()
{
    _pWindow = std::make_unique<Window>();

    if (_pWindow) {
        _pWindow->initialize();
    }
}

void Engine::_shutdownWindow()
{
    if (_pWindow) {
        _pWindow->shutdown();
        _pWindow.reset();
        _pWindow = nullptr;
    }
}

void Engine::_initializeGui()
{
    _pGui = std::make_unique<Gui>(_pWindow->handleSDL(), _pWindow->rendererSDL(), _pWindow->dpiScale());

    // register input events
}

void Engine::_shutdownGui()
{
    if (_pGui) {
        _pGui.reset();
        _pGui = nullptr;
    }
}

void Engine::_update()
{
}