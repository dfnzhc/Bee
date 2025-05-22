/**
 * @File EventTools.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/5/21
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include <qevent.h>

#include "Engine/Events/Event.hpp"
#include "Engine/Events/InputEvents.hpp"

namespace bee {

class EventTools
{
public:
    static Keys Map(Qt::Key key);
    static MouseButton Map(Qt::MouseButton button);
    static ModifierKeysState Map(Qt::KeyboardModifiers modifiers);

    static Ptr<KeyEvent> Map(QKeyEvent* event);
    static Ptr<MouseEvent> Map(QMouseEvent* event);
    static Ptr<MouseWheelEvent> Map(QWheelEvent* event);
};

} // namespace bee