/**
 * @File InputEvents.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/9
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Thirdparty.hpp"
#include "Core/Math/Math.hpp"

#include "Engine/Events/Event.hpp"

namespace bee {
enum class ModifierKey : u8
{
    None    = 0,
    Control = 1 << 0,
    Alt     = 1 << 1,
    Shift   = 1 << 2,
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

    bool areModifersDown(ModifierKey ModiferKeys) const;
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

class BEE_API InputEventBase : public IEventBase
{
public:
    /**
     * @brief Constructor for InputEventBase.
     * @param modifiers The state of modifier keys at the time of the event.
     */
    InputEventBase(const ModifierKeysState& modifiers)
        : _modifierKeys(modifiers)
    {
    }

    ~InputEventBase() override = default;

    /**
     * @brief Gets the state of the modifier keys (Shift, Ctrl, Alt).
     * @return Const reference to ModifierKeysState.
     */
    const ModifierKeysState& modifierKeys() const { return _modifierKeys; }

    // Helper methods to directly check modifier states

    // clang-format off
    bool isShiftDown()      const { return _modifierKeys.isShiftDown(); }
    bool isLeftShiftDown()  const { return _modifierKeys.isLeftShiftDown(); }
    bool isRightShiftDown() const { return _modifierKeys.isRightShiftDown(); }

    bool isControlDown()      const { return _modifierKeys.isControlDown(); }
    bool isLeftControlDown()  const { return _modifierKeys.isLeftControlDown(); }
    bool isRightControlDown() const { return _modifierKeys.isRightControlDown(); }

    bool isAltDown()      const { return _modifierKeys.isAltDown(); }
    bool isLeftAltDown()  const { return _modifierKeys.isLeftAltDown(); }
    bool isRightAltDown() const { return _modifierKeys.isRightAltDown(); }
    // clang-format on

protected:
    ModifierKeysState _modifierKeys;
};

/// ==========================
/// Key events
/// ==========================

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

/**
 * @brief Base class for specific keyboard events (Pressed, Released).
 *        Contains the key code involved in the event.
 */
class BEE_API KeyEvent : public InputEventBase
{
public:
    /**
     * @brief Constructor for KeyEvent.
     * @param key The key code.
     * @param modifiers The state of modifier keys.
     */
    KeyEvent(Keys key, const ModifierKeysState& modifiers)
        : InputEventBase(modifiers), _key(key)
    {
    }

    /**
     * @brief Gets the key involved in this event.
     * @return The Keys enum value.
     */
    Keys key() const { return _key; }

protected:
    Keys _key;
};

/**
 * @brief Event triggered when a key is pressed down.
 */
class BEE_API KeyPressedEvent final : public KeyEvent
{
public:
    /**
     * @brief Constructor for KeyPressedEvent.
     * @param key The key code that was pressed.
     * @param modifiers The state of modifier keys.
     * @param isRepeat True if this is a repeat event due to key being held down.
     */
    KeyPressedEvent(Keys key, const ModifierKeysState& modifiers = {}, bool isRepeat = false)
        : KeyEvent(key, modifiers), _bIsRepeat(isRepeat)
    {
    }

    /**
     * @brief Checks if this is a repeat event (key held down).
     * @return True if it's a repeat, false otherwise.
     */
    BEE_NODISCARD bool isRepeat() const { return _bIsRepeat; }

    BEE_NODISCARD std::type_index typeId() const override { return EventTypeId<KeyPressedEvent>(); }
    BEE_NODISCARD String toString() const override;

private:
    bool _bIsRepeat;
};

/**
 * @brief Event triggered when a key is released.
 */
class BEE_API KeyReleasedEvent final : public KeyEvent
{
public:
    /**
     * @brief Constructor for KeyReleasedEvent.
     * @param key The key code that was released.
     * @param modifiers The state of modifier keys at the time of release.
     */
    KeyReleasedEvent(Keys key, const ModifierKeysState& modifiers = {})
        : KeyEvent(key, modifiers)
    {
    }

    BEE_NODISCARD std::type_index typeId() const override { return EventTypeId<KeyReleasedEvent>(); }
    BEE_NODISCARD String toString() const override;
};

/// ==========================
/// Mouse events
/// ==========================

enum class MouseButton : u8
{
    Left,
    Middle,
    Right,
    X1,
    X2,
    Unknown // Any unknown mouse button
};

/**
 * @brief Base class for mouse button related events (Pressed, Released).
 */
class BEE_API MouseButtonEvent : public InputEventBase
{
public:
    /**
     * @brief Constructor for MouseButtonEvent.
     * @param button The mouse button involved.
     * @param x The x-coordinate of the mouse cursor.
     * @param y The y-coordinate of the mouse cursor.
     * @param modifiers The state of modifier keys.
     */
    MouseButtonEvent(MouseButton button, f32 x, f32 y, const ModifierKeysState& modifiers = {})
        : InputEventBase(modifiers), _x(x), _y(y), _button(button)
    {
    }

    BEE_NODISCARD MouseButton button() const { return _button; }
    BEE_NODISCARD f32 posX() const { return _x; }
    BEE_NODISCARD f32 posY() const { return _y; }
    BEE_NODISCARD vec2f position() const { return vec2f{_x, _y}; }

protected:
    f32 _x, _y;
    MouseButton _button;
};

/**
 * @brief Event triggered when a mouse button is pressed down.
 */
class BEE_API MouseButtonPressedEvent final : public MouseButtonEvent
{
public:
    /**
     * @brief Constructor for MouseButtonPressedEvent.
     * @param button The mouse button pressed.
     * @param x The x-coordinate of the mouse cursor.
     * @param y The y-coordinate of the mouse cursor.
     * @param clicks The number of clicks (1 for single, 2 for double, etc.).
     * @param modifiers The state of modifier keys.
     * @param isRepeat True if this is a "repeat" due to button being held (less common for mice).
     */
    MouseButtonPressedEvent(MouseButton button, f32 x, f32 y, u8 clicks, const ModifierKeysState& modifiers, bool isRepeat = false)
        : MouseButtonEvent(button, x, y, modifiers), _clicks(clicks), _bIsRepeat(isRepeat)
    {
    }

    BEE_NODISCARD u8 clicks() const { return _clicks; }
    BEE_NODISCARD bool isRepeat() const { return _bIsRepeat; }
    BEE_NODISCARD std::type_index typeId() const override { return EventTypeId<MouseButtonPressedEvent>(); }
    BEE_NODISCARD String toString() const override;

private:
    u8 _clicks;
    bool _bIsRepeat;
};

/**
 * @brief Event triggered when a mouse button is released.
 */
class BEE_API MouseButtonReleasedEvent final : public MouseButtonEvent
{
public:
    /**
     * @brief Constructor for MouseButtonReleasedEvent.
     * @param button The mouse button released.
     * @param x The x-coordinate of the mouse cursor.
     * @param y The y-coordinate of the mouse cursor.
     * @param modifiers The state of modifier keys.
     */
    MouseButtonReleasedEvent(MouseButton button, f32 x, f32 y, const ModifierKeysState& modifiers)
        : MouseButtonEvent(button, x, y, modifiers)
    {
    }

    BEE_NODISCARD std::type_index typeId() const override { return EventTypeId<MouseButtonReleasedEvent>(); }
    BEE_NODISCARD String toString() const override;
};

/**
 * @brief Event triggered when the mouse cursor moves.
 */
class BEE_API MouseMovedEvent final : public InputEventBase
{
public:
    /**
     * @brief Constructor for MouseMovedEvent.
     * @param x The new x-coordinate of the mouse cursor.
     * @param y The new y-coordinate of the mouse cursor.
     * @param deltaX The change in x-coordinate since the last move event.
     * @param deltaY The change in y-coordinate since the last move event.
     * @param modifiers The state of modifier keys.
     */
    MouseMovedEvent(f32 x, f32 y, f32 deltaX, f32 deltaY, const ModifierKeysState& modifiers)
        : InputEventBase(modifiers), _x(x), _y(y), _deltaX(deltaX), _deltaY(deltaY)
    {
    }

    BEE_NODISCARD f32 posX() const { return _x; }
    BEE_NODISCARD f32 posY() const { return _y; }
    BEE_NODISCARD vec2f position() const { return vec2f{_x, _y}; }

    BEE_NODISCARD f32 deltaX() const { return _deltaX; }
    BEE_NODISCARD f32 deltaY() const { return _deltaY; }
    BEE_NODISCARD vec2f deltaMotion() const { return vec2f{_deltaX, _deltaY}; }

    BEE_NODISCARD std::type_index typeId() const override { return EventTypeId<MouseMovedEvent>(); }
    BEE_NODISCARD String toString() const override;

private:
    f32 _x, _y;
    f32 _deltaX, _deltaY;
};

/**
 * @brief Event triggered when the mouse wheel is scrolled.
 */
class BEE_API MouseWheelEvent final : public InputEventBase
{
public:
    /**
     * @brief Constructor for MouseWheelEvent.
     * @param deltaX The horizontal scroll offset.
     * @param deltaY The vertical scroll offset.
     * @param modifiers The state of modifier keys.
     */
    MouseWheelEvent(f32 deltaX, f32 deltaY, const ModifierKeysState& modifiers)
        : InputEventBase(modifiers), _deltaX(deltaX), _deltaY(deltaY)
    {
    }

    /** Horizontal scroll */
    BEE_NODISCARD f32 deltaX() const { return _deltaX; }

    /** Vertical scroll */
    BEE_NODISCARD f32 deltaY() const { return _deltaY; }
    BEE_NODISCARD vec2f wheelDelta() const { return vec2f{_deltaX, _deltaY}; }

    BEE_NODISCARD std::type_index typeId() const override { return EventTypeId<MouseWheelEvent>(); }
    BEE_NODISCARD String toString() const override;

private:
    f32 _deltaX, _deltaY;
};

} // namespace bee