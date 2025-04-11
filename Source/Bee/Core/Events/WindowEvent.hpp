/**
 * @File WindowEvent.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/11
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Core/Events/Event.hpp"
#include "Math/Math.hpp"

namespace bee {

class BEE_API WindowResizeEvent final : public Event
{
public:
    WindowResizeEvent();
    WindowResizeEvent(int w, int h);

    ~WindowResizeEvent() override = default;

    vec2i extent() const;
    int width() const;
    int height() const;

    String toString() const override;

private:
    vec2i _extent = {};
};


} // namespace bee