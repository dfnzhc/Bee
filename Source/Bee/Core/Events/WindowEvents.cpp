/**
 * @File WindowEvents.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/11
 * @Brief This file is part of Bee.
 */

#include "Core/Events/WindowEvents.hpp"

using namespace bee;

WindowResizeEvent::WindowResizeEvent() : Event(EventType::WindowResize)
{
}

WindowResizeEvent::WindowResizeEvent(int w, int h) : Event(EventType::WindowResize), _extent(w, h)
{
}

vec2i WindowResizeEvent::extent() const
{
    return _extent;
}

int WindowResizeEvent::width() const
{
    return _extent.x;
}

int WindowResizeEvent::height() const
{
    return _extent.y;
}

String WindowResizeEvent::toString() const
{
    return std::format("Event - Window Resize: {}x{}", width(), height());
}