/**
 * @File Application.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */

#include "Core/App/Application.hpp"

#include "Core/App/Property.hpp"

#include "Core/Logger.hpp"
#include "Core/Error.hpp"

#include "Events/DispatchEvents.hpp"
#include "Events/EventManager.hpp"
#include "Events/InputEvents.hpp"
#include "Events/WindowEvents.hpp"

using namespace bee;

Application::Application()
{
}

Application::~Application()
{
    _shutdown();
}

int Application::execute()
{
    if (!_initialize()) {
        LogError("Application initialized failed.");
        return EXIT_FAILURE;
    }

    while (Property::IsEngineRunning()) {
        _update();

        EventManager::Instance().Process();
    }

    _shutdown();

    return EXIT_SUCCESS;
}

void Application::_registerInputEvent()
{
    const auto& em = EventManager::Instance();

    // clang-format off
    em.Register(EventType::Dispatch, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::Dispatch); _onDispatchEvent(event); });
    em.Register(EventType::keyboard, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::keyboard); _onKeyboardEvent(event); });
    em.Register(EventType::Mouse, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::Mouse); _onMouseEvent(event); });
    em.Register(EventType::WindowResize, [this](const EventPtr& event) { BEE_ASSERT(event->type() == EventType::WindowResize); _onWindowSizeChanged(event); });
    // clang-format on
}

void Application::_onDispatchEvent(const EventPtr& event) const
{
    const auto* dispatchEvent = dynamic_cast<const DispatchEvent*>(event.get());

    if (dispatchEvent->dispatchType() == DispatchEventType::Gui) {
        
    }
}

void Application::_onWindowSizeChanged(const EventPtr& event)
{
    const auto* winEvent = dynamic_cast<const WindowResizeEvent*>(event.get());

}

void Application::_onKeyboardEvent(const EventPtr& event)
{
    const auto* keyEvent = dynamic_cast<const KeyboardEvent*>(event.get());

    const auto key = keyEvent->key();

    if (key == Keys::Escape)
        Property::RequestEngineExit();
}

void Application::_onMouseEvent(const EventPtr& event)
{
    const auto* mouseEvent = dynamic_cast<const MouseEvent*>(event.get());
}

bool Application::_initialize()
{
    return true;
}

void Application::_shutdown()
{
}

void Application::_update()
{
}