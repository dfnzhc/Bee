/**
 * @File DispatchEvents.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/13
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Events/Event.hpp"

namespace bee {

enum class DispatchEventType
{
    Gui,
    Unknown
};

using VoidPtr = void*;

class BEE_API DispatchEvent final : public Event
{
public:
    DispatchEvent();
    DispatchEvent(DispatchEventType type, VoidPtr event);

    ~DispatchEvent() override = default;
    
    VoidPtr event() const;
    DispatchEventType dispatchType() const;

    String toString() const override;

private:
    VoidPtr _pEvent = nullptr;
    DispatchEventType _type = DispatchEventType::Unknown;
};




} // namespace bee