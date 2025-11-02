/**
 * @File EventPump.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/2
 * @Brief This file is part of Bee.
 */

#include "EventPump.hpp"
#include "SDLHeader.hpp"

using namespace Bee;

namespace
{
    KeyCode ConvertSDLKeycode(SDL_Keycode sdlKey)
    {
        // clang-format off
        switch (sdlKey)
        {
            case SDLK_A: return KeyCode::A;
            case SDLK_B: return KeyCode::B;
            case SDLK_C: return KeyCode::C;
            case SDLK_D: return KeyCode::D;
            case SDLK_E: return KeyCode::E;
            case SDLK_F: return KeyCode::F;
            case SDLK_G: return KeyCode::G;
            case SDLK_H: return KeyCode::H;
            case SDLK_I: return KeyCode::I;
            case SDLK_J: return KeyCode::J;
            case SDLK_K: return KeyCode::K;
            case SDLK_L: return KeyCode::L;
            case SDLK_M: return KeyCode::M;
            case SDLK_N: return KeyCode::N;
            case SDLK_O: return KeyCode::O;
            case SDLK_P: return KeyCode::P;
            case SDLK_Q: return KeyCode::Q;
            case SDLK_R: return KeyCode::R;
            case SDLK_S: return KeyCode::S;
            case SDLK_T: return KeyCode::T;
            case SDLK_U: return KeyCode::U;
            case SDLK_V: return KeyCode::V;
            case SDLK_W: return KeyCode::W;
            case SDLK_X: return KeyCode::X;
            case SDLK_Y: return KeyCode::Y;
            case SDLK_Z: return KeyCode::Z;

            case SDLK_0: return KeyCode::Num0;
            case SDLK_1: return KeyCode::Num1;
            case SDLK_2: return KeyCode::Num2;
            case SDLK_3: return KeyCode::Num3;
            case SDLK_4: return KeyCode::Num4;
            case SDLK_5: return KeyCode::Num5;
            case SDLK_6: return KeyCode::Num6;
            case SDLK_7: return KeyCode::Num7;
            case SDLK_8: return KeyCode::Num8;
            case SDLK_9: return KeyCode::Num9;

            case SDLK_F1: return KeyCode::F1;
            case SDLK_F2: return KeyCode::F2;
            case SDLK_F3: return KeyCode::F3;
            case SDLK_F4: return KeyCode::F4;
            case SDLK_F5: return KeyCode::F5;
            case SDLK_F6: return KeyCode::F6;
            case SDLK_F7: return KeyCode::F7;
            case SDLK_F8: return KeyCode::F8;
            case SDLK_F9: return KeyCode::F9;
            case SDLK_F10: return KeyCode::F10;
            case SDLK_F11: return KeyCode::F11;
            case SDLK_F12: return KeyCode::F12;

            case SDLK_LEFT: return KeyCode::Left;
            case SDLK_RIGHT: return KeyCode::Right;
            case SDLK_UP: return KeyCode::Up;
            case SDLK_DOWN: return KeyCode::Down;

            case SDLK_SPACE: return KeyCode::Space;
            case SDLK_RETURN: return KeyCode::Enter;
            case SDLK_ESCAPE: return KeyCode::Escape;
            case SDLK_TAB: return KeyCode::Tab;
            case SDLK_BACKSPACE: return KeyCode::Backspace;
            case SDLK_DELETE: return KeyCode::Delete;

            case SDLK_LSHIFT: return KeyCode::LeftShift;
            case SDLK_RSHIFT: return KeyCode::RightShift;
            case SDLK_LCTRL: return KeyCode::LeftCtrl;
            case SDLK_RCTRL: return KeyCode::RightCtrl;
            case SDLK_LALT: return KeyCode::LeftAlt;
            case SDLK_RALT: return KeyCode::RightAlt;
            case SDLK_LGUI: return KeyCode::LeftSuper;
            case SDLK_RGUI: return KeyCode::RightSuper;

            case SDLK_APOSTROPHE: return KeyCode::Apostrophe;
            case SDLK_COMMA: return KeyCode::Comma;
            case SDLK_MINUS: return KeyCode::Minus;
            case SDLK_PERIOD: return KeyCode::Period;
            case SDLK_SLASH: return KeyCode::Slash;
            case SDLK_BACKSLASH: return KeyCode::Backslash;
            case SDLK_SEMICOLON: return KeyCode::Semicolon;
            case SDLK_EQUALS: return KeyCode::Equals;
            case SDLK_LEFTBRACKET: return KeyCode::LeftBracket;
            case SDLK_RIGHTBRACKET: return KeyCode::RightBracket;
            case SDLK_GRAVE: return KeyCode::Grave;

            case SDLK_KP_0: return KeyCode::Keypad0;
            case SDLK_KP_1: return KeyCode::Keypad1;
            case SDLK_KP_2: return KeyCode::Keypad2;
            case SDLK_KP_3: return KeyCode::Keypad3;
            case SDLK_KP_4: return KeyCode::Keypad4;
            case SDLK_KP_5: return KeyCode::Keypad5;
            case SDLK_KP_6: return KeyCode::Keypad6;
            case SDLK_KP_7: return KeyCode::Keypad7;
            case SDLK_KP_8: return KeyCode::Keypad8;
            case SDLK_KP_9: return KeyCode::Keypad9;
            case SDLK_KP_PERIOD: return KeyCode::KeypadPeriod;
            case SDLK_KP_DIVIDE: return KeyCode::KeypadDivide;
            case SDLK_KP_MULTIPLY: return KeyCode::KeypadMultiply;
            case SDLK_KP_MINUS: return KeyCode::KeypadMinus;
            case SDLK_KP_PLUS: return KeyCode::KeypadPlus;
            case SDLK_KP_ENTER: return KeyCode::KeypadEnter;
            case SDLK_KP_EQUALS: return KeyCode::KeypadEquals;

            case SDLK_HOME: return KeyCode::Home;
            case SDLK_END: return KeyCode::End;
            case SDLK_PAGEUP: return KeyCode::PageUp;
            case SDLK_PAGEDOWN: return KeyCode::PageDown;
            case SDLK_INSERT: return KeyCode::Insert;
            case SDLK_CAPSLOCK: return KeyCode::CapsLock;
            case SDLK_SCROLLLOCK: return KeyCode::ScrollLock;
            case SDLK_NUMLOCKCLEAR: return KeyCode::NumLock;
            case SDLK_PRINTSCREEN: return KeyCode::PrintScreen;
            case SDLK_PAUSE: return KeyCode::Pause;
            case SDLK_MENU: return KeyCode::Menu;

            default: return KeyCode::Unknown;
        }
        // clang-format on
    }

    KeyModifier ConvertSDLModifiers(SDL_Keymod sdlMod)
    {
        KeyModifier modifiers = KeyModifier::None;

        // clang-format off
        if (sdlMod & SDL_KMOD_LSHIFT) modifiers |= KeyModifier::LeftShift;
        if (sdlMod & SDL_KMOD_RSHIFT) modifiers |= KeyModifier::RightShift;
        if (sdlMod & SDL_KMOD_LCTRL) modifiers |= KeyModifier::LeftCtrl;
        if (sdlMod & SDL_KMOD_RCTRL) modifiers |= KeyModifier::RightCtrl;
        if (sdlMod & SDL_KMOD_LALT) modifiers |= KeyModifier::LeftAlt;
        if (sdlMod & SDL_KMOD_RALT) modifiers |= KeyModifier::RightAlt;
        // clang-format on

        return modifiers;
    }

    MouseButton ConvertSDLMouseButton(u8 sdlButton)
    {
        // clang-format off
        switch (sdlButton)
        {
            case SDL_BUTTON_LEFT: return MouseButton::Left;
            case SDL_BUTTON_RIGHT: return MouseButton::Right;
            case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
            case SDL_BUTTON_X1: return MouseButton::X1;
            case SDL_BUTTON_X2: return MouseButton::X2;
            default: return MouseButton::Unknown;
        }
        // clang-format on
    }

    std::optional<PlatformEvent> ConvertSDLEvent(const SDL_Event& sdlEvent)
    {
        switch (sdlEvent.type)
        {
            case SDL_EVENT_QUIT:
            {
                CommonEvent event{};
                event.type = PlatformEventType::Quit;
                return PlatformEvent{event};
            }

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                KeyboardEvent event{};
                event.type = (sdlEvent.type == SDL_EVENT_KEY_DOWN) ? PlatformEventType::KeyDown : PlatformEventType::KeyUp;
                event.windowHandle = WindowHandle{sdlEvent.key.windowID};
                event.keyCode = ConvertSDLKeycode(sdlEvent.key.key);
                event.modifiers = ConvertSDLModifiers(sdlEvent.key.mod);
                event.rawScancode = static_cast<u16>(sdlEvent.key.scancode);
                event.isPressed = (sdlEvent.type == SDL_EVENT_KEY_DOWN);
                event.isRepeat = sdlEvent.key.repeat;
                return PlatformEvent{event};
            }

            case SDL_EVENT_TEXT_INPUT:
            {
                TextInputEvent event{};
                event.type         = PlatformEventType::TextInput;
                event.windowHandle = WindowHandle{sdlEvent.text.windowID};
                event.text         = std::string_view{sdlEvent.text.text};
                return PlatformEvent{event};
            }

            case SDL_EVENT_MOUSE_MOTION:
            {
                MouseMotionEvent event{};
                event.type         = PlatformEventType::MouseMotion;
                event.windowHandle = WindowHandle{sdlEvent.motion.windowID};
                event.x            = sdlEvent.motion.x;
                event.y            = sdlEvent.motion.y;
                event.deltaX       = sdlEvent.motion.xrel;
                event.deltaY       = sdlEvent.motion.yrel;
                return PlatformEvent{event};
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                MouseButtonEvent event{};
                event.type = (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                        ? PlatformEventType::MouseButtonDown
                        : PlatformEventType::MouseButtonUp;
                event.windowHandle = WindowHandle{sdlEvent.button.windowID};
                event.button       = ConvertSDLMouseButton(sdlEvent.button.button);
                event.x            = sdlEvent.button.x;
                event.y            = sdlEvent.button.y;
                event.clicks       = sdlEvent.button.clicks;
                event.isPressed    = (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
                return PlatformEvent{event};
            }

            case SDL_EVENT_MOUSE_WHEEL:
            {
                MouseWheelEvent event{};
                event.type         = PlatformEventType::MouseWheel;
                event.windowHandle = WindowHandle{sdlEvent.wheel.windowID};
                event.deltaX       = sdlEvent.wheel.x;
                event.deltaY       = sdlEvent.wheel.y;
                event.mouseX       = sdlEvent.wheel.mouse_x;
                event.mouseY       = sdlEvent.wheel.mouse_y;
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_SHOWN:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowShow;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_HIDDEN:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowHide;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_MOVED:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowMoved;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                event.data1  = sdlEvent.window.data1; // x position
                event.data2  = sdlEvent.window.data2; // y position
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_RESIZED:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowResize;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                event.data1  = sdlEvent.window.data1; // width
                event.data2  = sdlEvent.window.data2; // height
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_MINIMIZED:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowMinimized;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_MAXIMIZED:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowMaximized;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_MOUSE_ENTER:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowMouseEnter;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowMouseLeave;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowFocusGained;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_FOCUS_LOST:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowFocusLost;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowCloseRequested;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_WINDOW_DESTROYED:
            {
                WindowEvent event{};
                event.type   = PlatformEventType::WindowDestroyed;
                event.handle = WindowHandle{sdlEvent.window.windowID};
                return PlatformEvent{event};
            }

            case SDL_EVENT_DROP_FILE:
            {
                DropEvent event{};
                event.type         = PlatformEventType::DropFile;
                event.windowHandle = WindowHandle{sdlEvent.drop.windowID};
                event.data         = std::string{sdlEvent.drop.data ? sdlEvent.drop.data : ""};
                event.x            = sdlEvent.drop.x;
                event.y            = sdlEvent.drop.y;
                return PlatformEvent{event};
            }

            case SDL_EVENT_DROP_TEXT:
            {
                DropEvent event{};
                event.type         = PlatformEventType::DropText;
                event.windowHandle = WindowHandle{sdlEvent.drop.windowID};
                event.data         = std::string{sdlEvent.drop.data ? sdlEvent.drop.data : ""};
                event.x            = sdlEvent.drop.x;
                event.y            = sdlEvent.drop.y;
                return PlatformEvent{event};
            }

            default:
                // 不支持的事件类型，返回空
                return std::nullopt;
        }
    }
} // namespace


void EventPump::setEventCallback(EventCallback&& callback)
{
    _eventCallback = std::move(callback);
}

void EventPump::clearCallback()
{
    _eventCallback = {};
}

void EventPump::pumpEvents() const
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        auto platformEvent = ConvertSDLEvent(event);

        if (platformEvent.has_value() && _eventCallback)
        {
            _eventCallback(std::move(platformEvent.value()));
        }
    }
}
