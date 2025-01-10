/**
 * @File Keyboard.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/12/9
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Portability.hpp"
#include "Utility/Enum.hpp"
#include "Math/Hash.hpp"

#include <algorithm>
#include <fmt/format.h>

namespace bee {

struct KeyboardEvent
{
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
        Unknown     // Any unknown key code
    };

    enum class ModifierFlags : u8
    {
        None  = 0,
        Shift = 1,
        Ctrl  = 2,
        Alt   = 4
    };

    enum class Type : u8
    {
        Pressed,
        Released,
        Repeated,
        Input,
    };

    Type type; // 事件类型
    Key key;   // 按下/释放的键

    friend BEE_CONSTEXPR bool operator==(const KeyboardEvent& lhs, const KeyboardEvent rhs)
    {
        return lhs.type == rhs.type && lhs.key == rhs.key;
    }
    
    BEE_NODISCARD friend std::string ToString(const KeyboardEvent& ke)
    {
        return fmt::format("Keyboard [\"{}:{}\"]", me::enum_name(ke.type), me::enum_name(ke.key));
    }
};

class KeyboardInput
{
public:
    void onKeyboardEvent(const KeyboardEvent& keyboard) { _currState[keyboard.key] = keyboard; }

    void tick() { _prevState = _currState; }

    // clang-format off
    BEE_NODISCARD bool isKeyPressed(KeyboardEvent::Key key)  const { return TestKeyType(_currState, key, KeyboardEvent::Type::Pressed); }
    BEE_NODISCARD bool isKeyReleased(KeyboardEvent::Key key) const { return TestKeyType(_currState, key, KeyboardEvent::Type::Released); }
    BEE_NODISCARD bool isKeyRepeated(KeyboardEvent::Key key) const { return TestKeyType(_currState, key, KeyboardEvent::Type::Repeated); }
    BEE_NODISCARD bool isKeyInput(KeyboardEvent::Key key)    const { return TestKeyType(_currState, key, KeyboardEvent::Type::Input); }

    BEE_NODISCARD bool wasKeyPressed(KeyboardEvent::Key key)  const { return TestKeyType(_prevState, key, KeyboardEvent::Type::Pressed); }
    BEE_NODISCARD bool wasKeyReleased(KeyboardEvent::Key key) const { return TestKeyType(_prevState, key, KeyboardEvent::Type::Released); }
    BEE_NODISCARD bool wasKeyRepeated(KeyboardEvent::Key key) const { return TestKeyType(_prevState, key, KeyboardEvent::Type::Repeated); }
    BEE_NODISCARD bool wasKeyInput(KeyboardEvent::Key key)    const { return TestKeyType(_prevState, key, KeyboardEvent::Type::Input); }

    BEE_NODISCARD bool hasKeyPressed()  const { return TestKeyType(_currState, KeyboardEvent::Key::Unknown, KeyboardEvent::Type::Pressed); }
    BEE_NODISCARD bool hasKeyReleased() const { return TestKeyType(_currState, KeyboardEvent::Key::Unknown, KeyboardEvent::Type::Released); }
    BEE_NODISCARD bool hasKeyRepeated() const { return TestKeyType(_currState, KeyboardEvent::Key::Unknown, KeyboardEvent::Type::Repeated); }
    BEE_NODISCARD bool hasKeyInput()    const { return TestKeyType(_currState, KeyboardEvent::Key::Unknown, KeyboardEvent::Type::Input); }

    // clang-format on

    BEE_NODISCARD KeyboardEvent::ModifierFlags getModifiers() const
    {
        BEE_USE_MAGIC_ENUM_BIT_OPERATOR;
        KeyboardEvent::ModifierFlags flags = {};

        if (isKeyPressed(KeyboardEvent::Key::LeftShift) || isKeyPressed(KeyboardEvent::Key::RightShift))
            flags |= KeyboardEvent::ModifierFlags::Shift;

        if (isKeyPressed(KeyboardEvent::Key::LeftControl) || isKeyPressed(KeyboardEvent::Key::RightControl))
            flags |= KeyboardEvent::ModifierFlags::Ctrl;

        if (isKeyPressed(KeyboardEvent::Key::LeftAlt) || isKeyPressed(KeyboardEvent::Key::RightAlt))
            flags |= KeyboardEvent::ModifierFlags::Alt;

        return flags;
    }

    BEE_NODISCARD bool hasModifier() const { return getModifiers() != KeyboardEvent::ModifierFlags::None; }

    BEE_NODISCARD bool hasModifier(KeyboardEvent::ModifierFlags modifier) const
    {
        BEE_USE_MAGIC_ENUM_BIT_OPERATOR;
        return (getModifiers() & modifier) != KeyboardEvent::ModifierFlags::None;
    }

private:
    using KeyState = std::unordered_map<KeyboardEvent::Key, KeyboardEvent>;

    BEE_NODISCARD inline static bool TestKeyType(const KeyState& ks, KeyboardEvent::Key key, KeyboardEvent::Type kt)
    {
        if (key < KeyboardEvent::Key::Unknown)
            return ks.contains(key) && ks.at(key).type == kt;

        return std::ranges::any_of(ks, [kt](const auto& p) { return p.second.type == kt; });
    }

private:
    KeyState _prevState = {}, _currState = {};
};

} // namespace bee

namespace std {

template<> struct hash<bee::KeyboardEvent>
{
    size_t operator()(const bee::KeyboardEvent& ke) const noexcept
    {
        size_t seed = 0;
        bee::HashCombine(seed, magic_enum::enum_name(ke.type));
        bee::HashCombine(seed, magic_enum::enum_name(ke.key));

        return seed;
    }
};
} // namespace std