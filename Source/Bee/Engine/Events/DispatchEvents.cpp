/**
 * @File DispatchEvents.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/13
 * @Brief This file is part of Bee.
 */

#include "Engine/Events/DispatchEvents.hpp"

using namespace bee;
#if 0

DispatchEvent::DispatchEvent() : Event(EventType::Dispatch)
{
}

DispatchEvent::DispatchEvent(DispatchEventType type, VoidPtr event) : Event(EventType::Dispatch), _pEvent(event), _type(type)
{
}

VoidPtr DispatchEvent::event() const
{
    return _pEvent;
}

DispatchEventType DispatchEvent::dispatchType() const
{
    return _type;
}

String DispatchEvent::toString() const
{
    return std::format("Event - Dispatch: {}", ToString(_type));
}
#endif