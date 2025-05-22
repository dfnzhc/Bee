/**
 * @File EventTools.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/5/21
 * @Brief This file is part of Bee.
 */

#include "Utils/EventTools.hpp"

#include "Core/Error.hpp"

using namespace bee;

Keys EventTools::Map(Qt::Key key)
{
    // clang-format off
    switch (key) {
    case Qt::Key_Space:        return Keys::Space;
    case Qt::Key_Apostrophe:   return Keys::Apostrophe;
    case Qt::Key_Comma:        return Keys::Comma;
    case Qt::Key_Minus:        return Keys::Minus;
    case Qt::Key_Period:       return Keys::Period;
    case Qt::Key_Slash:        return Keys::Slash;
    case Qt::Key_0:            return Keys::Key0;
    case Qt::Key_1:            return Keys::Key1;
    case Qt::Key_2:            return Keys::Key2;
    case Qt::Key_3:            return Keys::Key3;
    case Qt::Key_4:            return Keys::Key4;
    case Qt::Key_5:            return Keys::Key5;
    case Qt::Key_6:            return Keys::Key6;
    case Qt::Key_7:            return Keys::Key7;
    case Qt::Key_8:            return Keys::Key8;
    case Qt::Key_9:            return Keys::Key9;
    case Qt::Key_Semicolon:    return Keys::Semicolon;
    case Qt::Key_Equal:        return Keys::Equal;
    case Qt::Key_A:            return Keys::A;
    case Qt::Key_B:            return Keys::B;
    case Qt::Key_C:            return Keys::C;
    case Qt::Key_D:            return Keys::D;
    case Qt::Key_E:            return Keys::E;
    case Qt::Key_F:            return Keys::F;
    case Qt::Key_G:            return Keys::G;
    case Qt::Key_H:            return Keys::H;
    case Qt::Key_I:            return Keys::I;
    case Qt::Key_J:            return Keys::J;
    case Qt::Key_K:            return Keys::K;
    case Qt::Key_L:            return Keys::L;
    case Qt::Key_M:            return Keys::M;
    case Qt::Key_N:            return Keys::N;
    case Qt::Key_O:            return Keys::O;
    case Qt::Key_P:            return Keys::P;
    case Qt::Key_Q:            return Keys::Q;
    case Qt::Key_R:            return Keys::R;
    case Qt::Key_S:            return Keys::S;
    case Qt::Key_T:            return Keys::T;
    case Qt::Key_U:            return Keys::U;
    case Qt::Key_V:            return Keys::V;
    case Qt::Key_W:            return Keys::W;
    case Qt::Key_X:            return Keys::X;
    case Qt::Key_Y:            return Keys::Y;
    case Qt::Key_Z:            return Keys::Z;
    case Qt::Key_BracketLeft:  return Keys::LeftBracket;
    case Qt::Key_Backslash:    return Keys::Backslash;
    case Qt::Key_BracketRight: return Keys::RightBracket;
    case Qt::Key_QuoteLeft:    return Keys::GraveAccent;

    case Qt::Key_Escape:       return Keys::Escape;
    case Qt::Key_Tab:          return Keys::Tab;
    case Qt::Key_Enter:
    case Qt::Key_Return:       return Keys::Enter;
    case Qt::Key_Backspace:    return Keys::Backspace;
    case Qt::Key_Insert:       return Keys::Insert;
    case Qt::Key_Delete:       return Keys::Del;
    case Qt::Key_Right:        return Keys::Right;
    case Qt::Key_Left:         return Keys::Left;
    case Qt::Key_Down:         return Keys::Down;
    case Qt::Key_Up:           return Keys::Up;
    case Qt::Key_PageUp:       return Keys::PageUp;
    case Qt::Key_PageDown:     return Keys::PageDown;
    case Qt::Key_Home:         return Keys::Home;
    case Qt::Key_End:          return Keys::End;
    case Qt::Key_CapsLock:     return Keys::CapsLock;
    case Qt::Key_ScrollLock:   return Keys::ScrollLock;
    case Qt::Key_NumLock:      return Keys::NumLock;
    case Qt::Key_Print:        return Keys::PrintScreen;
    case Qt::Key_Pause:        return Keys::Pause;
    case Qt::Key_F1:           return Keys::F1;
    case Qt::Key_F2:           return Keys::F2;
    case Qt::Key_F3:           return Keys::F3;
    case Qt::Key_F4:           return Keys::F4;
    case Qt::Key_F5:           return Keys::F5;
    case Qt::Key_F6:           return Keys::F6;
    case Qt::Key_F7:           return Keys::F7;
    case Qt::Key_F8:           return Keys::F8;
    case Qt::Key_F9:           return Keys::F9;
    case Qt::Key_F10:          return Keys::F10;
    case Qt::Key_F11:          return Keys::F11;
    case Qt::Key_F12:          return Keys::F12;

    case Qt::Key_multiply:     return Keys::KeypadMultiply;
    case Qt::Key_division:     return Keys::KeypadDivide;

    case Qt::Key_Super_L:      return Keys::LeftSuper;
    case Qt::Key_Super_R:      return Keys::RightSuper;
    case Qt::Key_Menu:         return Keys::Menu;
    case Qt::Key_Shift:        return Keys::LeftShift;
    case Qt::Key_Control:      return Keys::LeftControl;
    case Qt::Key_Alt:          return Keys::LeftAlt;
        
    default: ;
    }
    // clang-format on

    return Keys::Unknown;
}

MouseButton EventTools::Map(Qt::MouseButton button)
{
    switch (button) {
    case Qt::LeftButton: return MouseButton::Left;
    case Qt::RightButton: return MouseButton::Right;
    case Qt::MiddleButton: return MouseButton::Middle;
    case Qt::XButton1: return MouseButton::X1;
    case Qt::XButton2: return MouseButton::X2;
    default: ;
    }

    return MouseButton::Unknown;
}

ModifierKeysState EventTools::Map(Qt::KeyboardModifiers modifiers)
{
    const auto leftShift   = (modifiers & Qt::ShiftModifier) != Qt::NoModifier;
    const auto leftControl = (modifiers & Qt::ControlModifier) != Qt::NoModifier;
    const auto leftAlt     = (modifiers & Qt::AltModifier) != Qt::NoModifier;

    return {leftShift, false, leftControl, false, leftAlt, false};
}

Ptr<KeyEvent> EventTools::Map(QKeyEvent* event)
{
    const auto key = Map(static_cast<Qt::Key>(event->key()));
    const auto modifiers = Map(event->modifiers());

    if (event->type() == QEvent::Type::KeyPress) {
        return MakePtr<KeyPressedEvent>(key, modifiers);
    }
    
    if (event->type() == QEvent::Type::KeyRelease) {
        return MakePtr<KeyReleasedEvent>(key, modifiers);
    }
    
    BEE_UNREACHABLE();
    return nullptr;
}

Ptr<MouseEvent> EventTools::Map(QMouseEvent* event)
{
    const auto button    = Map(event->button());
    const auto x         = event->pos().x();
    const auto y         = event->pos().y();
    const auto modifiers = Map(event->modifiers());

    if (event->type() == QEvent::Type::MouseMove) {

        return MakePtr<MouseMovedEvent>(x, y);
    }
    
    if (event->type() == QEvent::Type::MouseButtonPress ||
        event->type() == QEvent::Type::MouseButtonDblClick) {
        const u8 clicks = event->type() == QEvent::Type::MouseButtonDblClick ? 2 : 1;

        return MakePtr<MouseButtonPressedEvent>(button, x, y, clicks, modifiers);
    }
    
    if (event->type() == QEvent::Type::MouseButtonRelease) {
        return MakePtr<MouseButtonReleasedEvent>(button, x, y, modifiers);
    }

    BEE_UNREACHABLE();
    return nullptr;
}

Ptr<MouseWheelEvent> EventTools::Map(QWheelEvent* event)
{
    const auto dx         = event->angleDelta().x();
    const auto dy         = event->angleDelta().y();
    const auto modifiers = Map(event->modifiers());
    
    return MakePtr<MouseWheelEvent>(dx, dy, modifiers);
}