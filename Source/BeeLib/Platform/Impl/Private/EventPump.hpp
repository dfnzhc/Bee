/**
 * @File EventPump.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/2
 * @Brief This file is part of Bee.
 */

#pragma once

#include <functional>
#include "PlatformTypes.hpp"

namespace Bee
{
    class EventPump final
    {
    public:
        using EventCallback = std::function<void(PlatformEvent&&)>;

        void setEventCallback(EventCallback&& callback);
        void clearCallback();

        void pumpEvents() const;

    private:
        EventCallback _eventCallback;
    };
} // namespace Bee
