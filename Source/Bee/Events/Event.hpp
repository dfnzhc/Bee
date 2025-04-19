/**
 * @File Event.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/7
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Memory/Memory.hpp"
#include "Core/Thirdparty.hpp"

namespace bee {
enum class EventType
{
    keyboard,
    Mouse,
    DragFile,
    WindowResize,
    Dispatch,
    Unknown
};

// clang-format off
class BEE_API Event
{
public:
    explicit Event(EventType inType) : _type(inType) { }
    virtual ~Event() = default;

    EventType type() const { return _type; }
    
    virtual String toString() const { return "Unknown Event"; } 

private:
    EventType _type;
};
// clang-format on

template<typename T>
concept cIsEvent = requires(T t)
{
    { t.type() } -> std::convertible_to<EventType>;
    std::is_base_of_v<Event, T>;
};

using EventPtr = Ptr<Event>;

} // namespace bee