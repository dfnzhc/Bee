/**
 * @File Engine.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */

#include "Engine/Engine.hpp"

#include "Core/Utility/Property.hpp"

#include "Core/Logger.hpp"
#include "Core/Error.hpp"

#include "Engine/EventManager.hpp"
#include "Engine/Events/DispatchEvents.hpp"
#include "Engine/Events/InputEvents.hpp"
#include "Engine/Events/WindowEvents.hpp"

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
        _update();

        // EventManager::Instance().Process();
    }

    _shutdown();

    return EXIT_SUCCESS;
}

void Engine::_registerInputEvent()
{
    // const auto& em = EventManager::Instance();
    //
    // // clang-format off
    // em.Register(EventType::Dispatch, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::Dispatch); _onDispatchEvent(event); });
    // em.Register(EventType::keyboard, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::keyboard); _onKeyboardEvent(event); });
    // em.Register(EventType::Mouse, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::Mouse); _onMouseEvent(event); });
    // em.Register(EventType::WindowResize, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::WindowResize); _onWindowSizeChanged(event); });
    // // clang-format on
}

void Engine::_onDispatchEvent(const EventPtr& event) const
{
    // const auto* dispatchEvent = dynamic_cast<const DispatchEvent*>(event.get());
    //
    // if (dispatchEvent->dispatchType() == DispatchEventType::Gui) {
    // }
}

void Engine::_onWindowSizeChanged(const EventPtr& event)
{
    // const auto* winEvent = dynamic_cast<const WindowResizeEvent*>(event.get());
}

void Engine::_onKeyboardEvent(const EventPtr& event)
{
    // const auto* keyEvent = dynamic_cast<const KeyboardEvent*>(event.get());
    //
    // const auto key = keyEvent->key();
    //
    // if (key == Keys::Escape)
    //     Property::RequestEngineExit();
}

void Engine::_onMouseEvent(const EventPtr& event)
{
    // const auto* mouseEvent = dynamic_cast<const MouseEvent*>(event.get());
}

bool Engine::_initialize()
{
    return true;
}

void Engine::_shutdown()
{
}

void Engine::_update()
{
}