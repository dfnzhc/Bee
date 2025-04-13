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
#include "Events/DispatchEvents.hpp"
#include "Events/EventManager.hpp"
#include "Events/InputEvents.hpp"
#include "Events/WindowEvents.hpp"

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
        _drawFrame();

        EventManager::Instance().Process();
    }

    _shutdown();

    return EXIT_SUCCESS;
}

void Engine::_registerInputEvent()
{
    const auto& em = EventManager::Instance();

    // clang-format off
    em.Register(EventType::Dispatch, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::Dispatch); _onDispatchEvent(event); });
    em.Register(EventType::keyboard, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::keyboard); _onKeyboardEvent(event); });
    em.Register(EventType::Mouse, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::Mouse); _onMouseEvent(event); });
    em.Register(EventType::WindowResize, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::WindowResize); _onWindowSizeChanged(event); });
    // clang-format on
}

void Engine::_onDispatchEvent(const EventPtr& event) const
{
    const auto* dispatchEvent = dynamic_cast<const DispatchEvent*>(event.get());

    if (dispatchEvent->dispatchType() == DispatchEventType::Gui) {
        if (_pGui)
            _pGui->handleEvents(dispatchEvent->event());
    }
}

void Engine::_onWindowSizeChanged(const EventPtr& event)
{
    const auto* winEvent = dynamic_cast<const WindowResizeEvent*>(event.get());

}

void Engine::_onKeyboardEvent(const EventPtr& event)
{
    const auto* keyEvent = dynamic_cast<const KeyboardEvent*>(event.get());

    const auto key = keyEvent->key();

    if (key == Keys::Escape)
        Property::RequestEngineExit();
}

void Engine::_onMouseEvent(const EventPtr& event)
{
    const auto* mouseEvent = dynamic_cast<const MouseEvent*>(event.get());
}

bool Engine::_initialize()
{
    _initializeWindow();
    SimpleCheckAndReturn(_pWindow);

    _initializeGui();
    SimpleCheckAndReturn(_pGui);

    _registerInputEvent();

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

void Engine::_drawFrame()
{
    _pGui->beginFrame();

    // gui test demo

    _pGui->present();
}