/**
 * @File Inputs.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include <bitset>
#include "Core/Defines.hpp"
#include "Core/Portability.hpp"
#include "Math/Constant.hpp"
#include "Utility/Enum.hpp"
#include "Utility/Assert.hpp"

namespace bee {

enum class MouseButton : u8
{
    Left,
    Middle,
    Right,
    Count
};

enum class Key : u32
{
    Space        = ' ',
    Apostrophe   = '\'',
    Comma        = ',',
    Minus        = '-',
    Period       = '.',
    Slash        = '/',
    Key0         = '0',
    Key1         = '1',
    Key2         = '2',
    Key3         = '3',
    Key4         = '4',
    Key5         = '5',
    Key6         = '6',
    Key7         = '7',
    Key8         = '8',
    Key9         = '9',
    Semicolon    = ';',
    Equal        = '=',
    A            = 'A',
    B            = 'B',
    C            = 'C',
    D            = 'D',
    E            = 'E',
    F            = 'F',
    G            = 'G',
    H            = 'H',
    I            = 'I',
    J            = 'J',
    K            = 'K',
    L            = 'L',
    M            = 'M',
    N            = 'N',
    O            = 'O',
    P            = 'P',
    Q            = 'Q',
    R            = 'R',
    S            = 'S',
    T            = 'T',
    U            = 'U',
    V            = 'V',
    W            = 'W',
    X            = 'X',
    Y            = 'Y',
    Z            = 'Z',
    LeftBracket  = '[',
    Backslash    = '\\',
    RightBracket = ']',
    GraveAccent  = '`',

    Escape = 256,
    Tab,
    Enter,
    Backspace,
    Insert,
    Del,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Keypad0,
    Keypad1,
    Keypad2,
    Keypad3,
    Keypad4,
    Keypad5,
    Keypad6,
    Keypad7,
    Keypad8,
    Keypad9,
    KeypadDel,
    KeypadDivide,
    KeypadMultiply,
    KeypadSubtract,
    KeypadAdd,
    KeypadEnter,
    KeypadEqual,
    LeftShift,
    LeftControl,
    LeftAlt,
    LeftSuper, // Windows key on windows
    RightShift,
    RightControl,
    RightAlt,
    RightSuper, // Windows key on windows
    Menu,
    Unknown,    // Any unknown key code

    Count,
};

enum class ModifierFlags : u8
{
    None  = 0,
    Shift = 1,
    Ctrl  = 2,
    Alt   = 4
};

using Modifiers = mec::bitset<ModifierFlags>;

struct MouseEvent
{
    enum class Type
    {
        ButtonDown,
        ButtonUp,
        Move,
        Wheel,
    };

    Type type;
    vec2 pos;           // 标准化坐标，范围[0, 1]
    vec2 screenPos;     // 屏幕空间坐标，范围[0, 窗口大小]
    vec2 wheelDelta;    // 鼠标滚轮滚动量
    Modifiers mods;     // 键盘修饰键标记
    MouseButton button; // 鼠标按钮
                        
    BEE_NODISCARD bool hasModifier(ModifierFlags mod) const { return mods.test(mod); }
};

struct KeyboardEvent
{
    enum class Type
    {
        KeyPressed,
        KeyReleased,
        KeyRepeated,
        Input,
    };

    Type type;         // 事件类型
    Key key;           // 按下/释放的键
    Modifiers mods;    // 键盘修饰键标记
    u32 codepoint = 0; // UTF-32 码点

    BEE_NODISCARD bool hasModifier(ModifierFlags mod) const { return mods.test(mod); }
};

class InputState
{
public:
    // clang-format off
    BEE_NODISCARD bool isMouseMoving()                       const { return _mouseMoving; }
    BEE_NODISCARD bool isMouseButtonDown(MouseButton mb)     const { return _currentMouseState.contains(mb); }
    BEE_NODISCARD bool isMouseButtonClicked(MouseButton mb)  const { return _currentMouseState.contains(mb) && !_previousMouseState.contains(mb); }
    BEE_NODISCARD bool isMouseButtonReleased(MouseButton mb) const { return !_currentMouseState.contains(mb) && _previousMouseState.contains(mb); }

    BEE_NODISCARD bool isKeyDown(Key key)     const { return _currentKeyState.contains(key); }
    BEE_NODISCARD bool isKeyPressed(Key key)  const { return _currentKeyState.contains(key) && !_previousKeyState.contains(key); }
    BEE_NODISCARD bool isKeyReleased(Key key) const { return !_currentKeyState.contains(key) && _previousKeyState.contains(key); }

    BEE_NODISCARD bool isModifierDown(ModifierFlags mod)     const { return getModifierState(_currentKeyState, mod); }
    BEE_NODISCARD bool isModifierPressed(ModifierFlags mod)  const { return getModifierState(_currentKeyState, mod) && !getModifierState(_previousKeyState, mod); }
    BEE_NODISCARD bool isModifierReleased(ModifierFlags mod) const { return !getModifierState(_currentKeyState, mod) && getModifierState(_previousKeyState, mod); }

    // clang-format on

    void onKeyEvent(const KeyboardEvent& keyEvent)
    {
        if (keyEvent.type == KeyboardEvent::Type::KeyPressed) {
            _currentKeyState.insert(keyEvent.key);
        }
        else if (keyEvent.type == KeyboardEvent::Type::KeyReleased) {
            _currentKeyState.extract(keyEvent.key);
        }
    }

    void onMouseEvent(const MouseEvent& mouseEvent)
    {
        if (mouseEvent.type == MouseEvent::Type::ButtonDown) {
            _currentMouseState.insert(mouseEvent.button);
        }
        else if (mouseEvent.type == MouseEvent::Type::ButtonUp) {
            _currentMouseState.extract(mouseEvent.button);
        }
        else if (mouseEvent.type == MouseEvent::Type::Move) {
            _mouseMoving = true;
        }
    }

    void endFrame()
    {
        _previousKeyState   = _currentKeyState;
        _previousMouseState = _currentMouseState;

        _mouseMoving = false;
    }

private:
    using KeyStates  = std::set<Key>;
    using MouseState = std::set<MouseButton>;

    BEE_NODISCARD bool getModifierState(const KeyStates& keys, ModifierFlags mod) const
    {
        switch (mod) {
        case ModifierFlags::Shift : return keys.contains(Key::LeftShift) || keys.contains(Key::RightShift);
        case ModifierFlags::Ctrl  : return keys.contains(Key::LeftControl) || keys.contains(Key::RightControl);
        case ModifierFlags::Alt   : return keys.contains(Key::LeftAlt) || keys.contains(Key::RightAlt);
        case ModifierFlags::None  : break;
        }

        return false;
    }

    KeyStates _currentKeyState     = {};
    KeyStates _previousKeyState    = {};
    MouseState _currentMouseState  = {};
    MouseState _previousMouseState = {};

    bool _mouseMoving = false;
};

} // namespace bee