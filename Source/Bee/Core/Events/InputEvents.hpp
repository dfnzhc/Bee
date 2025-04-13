/**
 * @File InputEvents.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/9
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Thirdparty.hpp"
#include "Core/Events/Event.hpp"

#include "Math/Math.hpp"

namespace bee {

enum class ModifierKey : u8
{
    None    = 0,
    Control = 1 << 0,
    Alt     = 1 << 1,
    Shift   = 1 << 2,
};

enum class Keys : u32
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

    Escape         = 256,
    Tab            = 257,
    Enter          = 258,
    Backspace      = 259,
    Insert         = 260,
    Del            = 261,
    Right          = 262,
    Left           = 263,
    Down           = 264,
    Up             = 265,
    PageUp         = 266,
    PageDown       = 267,
    Home           = 268,
    End            = 269,
    CapsLock       = 270,
    ScrollLock     = 271,
    NumLock        = 272,
    PrintScreen    = 273,
    Pause          = 274,
    F1             = 275,
    F2             = 276,
    F3             = 277,
    F4             = 278,
    F5             = 279,
    F6             = 280,
    F7             = 281,
    F8             = 282,
    F9             = 283,
    F10            = 284,
    F11            = 285,
    F12            = 286,
    Keypad0        = 287,
    Keypad1        = 288,
    Keypad2        = 289,
    Keypad3        = 290,
    Keypad4        = 291,
    Keypad5        = 292,
    Keypad6        = 293,
    Keypad7        = 294,
    Keypad8        = 295,
    Keypad9        = 296,
    KeypadDel      = 297,
    KeypadDivide   = 298,
    KeypadMultiply = 299,
    KeypadSubtract = 300,
    KeypadAdd      = 301,
    KeypadEnter    = 302,
    KeypadEqual    = 303,
    LeftShift      = 304,
    LeftControl    = 305,
    LeftAlt        = 306,
    LeftSuper      = 307, // Windows key on windows
    RightShift     = 308,
    RightControl   = 309,
    RightAlt       = 310,
    RightSuper     = 311, // Windows key on windows
    Menu           = 312,
    Unknown        = 313 // Any unknown key code
};

enum class MouseButton : u8
{
    Left,
    Middle,
    Right,
    X1,
    X2,
    Unknown // Any unknown mouse button
};

enum class InputType : u16
{
    KeyDown,
    KeyUp,

    MouseButtonDown,
    MouseButtonUp,
    MouseMotion,
    MouseWheel,

    Unknown
};

struct BEE_API ModifierKeysState
{
public:
    ModifierKeysState();
    ModifierKeysState(bool bInIsLeftShiftDown,
                      bool bInIsRightShiftDown,
                      bool bInIsLeftControlDown,
                      bool bInIsRightControlDown,
                      bool bInIsLeftAltDown,
                      bool bInIsRightAltDown);

    // clang-format off
    bool isShiftDown()      const;
    bool isLeftShiftDown()  const;
    bool isRightShiftDown() const;

    bool isControlDown()      const;
    bool isLeftControlDown()  const;
    bool isRightControlDown() const;

    bool isAltDown()      const;
    bool isLeftAltDown()  const;
    bool isRightAltDown() const;
    // clang-format on

    bool AreModifersDown(ModifierKey ModiferKeys) const;
    bool anyModifiersDown() const;

    String toString() const;

private:
    u16 _bIsLeftShiftDown  : 1;
    u16 _bIsRightShiftDown : 1;

    u16 _bIsLeftControlDown  : 1;
    u16 _bIsRightControlDown : 1;

    u16 _bIsLeftAltDown  : 1;
    u16 _bIsRightAltDown : 1;
};

class BEE_API InputEvents
{
public:
    // clang-format off
    InputEvents() = default;
    InputEvents(const ModifierKeysState& modifierKeys, bool bInIsRepeat, InputType inputType) : _modifierKeys(modifierKeys), _inputType(inputType), _bIsRepeat(bInIsRepeat) { }
    
    virtual ~InputEvents() = default;

    bool isRepeat() const { return _bIsRepeat; }

    bool isShiftDown()      const { return _modifierKeys.isShiftDown(); }
    bool isLeftShiftDown()  const { return _modifierKeys.isLeftShiftDown(); }
    bool isRightShiftDown() const { return _modifierKeys.isRightShiftDown(); }

    bool isControlDown()      const { return _modifierKeys.isControlDown(); }
    bool isLeftControlDown()  const { return _modifierKeys.isLeftControlDown(); }
    bool isRightControlDown() const { return _modifierKeys.isRightControlDown(); }

    bool isAltDown()      const { return _modifierKeys.isAltDown(); }
    bool isLeftAltDown()  const { return _modifierKeys.isLeftAltDown(); }
    bool isRightAltDown() const { return _modifierKeys.isRightAltDown(); }

    const ModifierKeysState& modifierKeys() const { return _modifierKeys; }
    InputType inputType() const { return _inputType; }
    // clang-format on
    
protected:
    ModifierKeysState _modifierKeys = {};

    InputType _inputType = InputType::Unknown;
    bool _bIsRepeat      = false;
};

class BEE_API KeyboardEvent final : public Event, public InputEvents
{
public:
    KeyboardEvent() : Event(EventType::keyboard), InputEvents()
    {
    }

    KeyboardEvent(Keys key, InputType keyType, const ModifierKeysState& modifierKeys, bool bIsRepeat) : Event(EventType::keyboard), InputEvents(modifierKeys, bIsRepeat, keyType), _key(key)
    {
    }

    ~KeyboardEvent() override = default;

    Keys key() const { return _key; }
    String toString() const override;

private:
    Keys _key = Keys::Unknown;
    // Keycode?
};

class BEE_API MouseEvent final : public Event, public InputEvents
{
public:
    MouseEvent();

    // Button
    MouseEvent(f32 x, f32 y, MouseButton btn, InputType mouseType, u8 clicks, const ModifierKeysState& modifierKeys, bool bIsRepeat = false);
    // Motion
    MouseEvent(f32 x, f32 y, f32 relX, f32 relY, const ModifierKeysState& modifierKeys);
    // Wheel
    MouseEvent(f32 h, f32 v, const ModifierKeysState& modifierKeys);

    ~MouseEvent() override = default;

    Opt<MouseButton> button() const;

    Opt<vec2f> position() const;
    Opt<vec2f> relativeMotion() const;
    Opt<vec2f> wheelDelta() const;

    Opt<u8> clicks() const;
    
    bool isButtonEvent() const; 

    String toString() const override;

private:
    f32 _posX = {}, _posY = {}; ///< normalized position [0, 1]
    f32 _relX = {}, _relY = {};

    MouseButton _button = MouseButton::Unknown;
    u8 _clicks          = {};
};

} // namespace bee