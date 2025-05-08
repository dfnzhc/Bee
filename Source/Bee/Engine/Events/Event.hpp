/**
 * @File Event.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/7
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Memory/Memory.hpp"
#include "Core/Thirdparty.hpp"

#include <typeindex>

namespace bee {

/**
 * @class IEventBase
 * @brief Base interface for all event types within the event system.
 */
class IEventBase
{
public:
    virtual ~IEventBase() = default;

    /**
     * @brief Retrieves the runtime type index of this event instance.
     * @return std::type_index representing the concrete type of the event.
     */
    virtual std::type_index typeId() const = 0;

    /**
     * @brief Converts the event's information into a human-readable string.
     * @return A string representing the event's data. Useful for logging and debugging.
     */
    virtual String toString() const { return "Unknown Event"; }

protected:
    IEventBase() = default;
};

using EventPtr = Ptr<IEventBase>;

template<typename E>
concept cEventType = requires(E t)
{
    { t.typeId() } -> std::convertible_to<std::type_index>;
    std::is_base_of_v<IEventBase, E>;
};

/**
 * @brief Helper template function to get the static type_id for any event type E.
 * @tparam E The event type, must be derived from IEventBase.
 * @return The std::type_index for event type E.
 */
template<cEventType E>
constexpr std::type_index EventTypeId()
{
    return std::type_index(typeid(E));
}
} // namespace bee