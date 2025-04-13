/**
 * @File InputEvent.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/10
 * @Brief This file is part of Bee.
 */

#include "Core/Events/InputEvent.hpp"

using namespace bee;

ModifierKeysState::ModifierKeysState(bool bInIsLeftShiftDown,
                                     bool bInIsRightShiftDown,
                                     bool bInIsLeftControlDown,
                                     bool bInIsRightControlDown,
                                     bool bInIsLeftAltDown,
                                     bool bInIsRightAltDown)
    : _bIsLeftShiftDown(bInIsLeftShiftDown),
      _bIsRightShiftDown(bInIsRightShiftDown),
      _bIsLeftControlDown(bInIsLeftControlDown),
      _bIsRightControlDown(bInIsRightControlDown),
      _bIsLeftAltDown(bInIsLeftAltDown),
      _bIsRightAltDown(bInIsRightAltDown)
{
}

ModifierKeysState::ModifierKeysState()
    : _bIsLeftShiftDown(false),
      _bIsRightShiftDown(false),
      _bIsLeftControlDown(false),
      _bIsRightControlDown(false),
      _bIsLeftAltDown(false),
      _bIsRightAltDown(false)
{
}

bool ModifierKeysState::isShiftDown() const { return _bIsLeftShiftDown || _bIsRightShiftDown; }
bool ModifierKeysState::isLeftShiftDown() const { return _bIsLeftShiftDown; }
bool ModifierKeysState::isRightShiftDown() const { return _bIsRightShiftDown; }

bool ModifierKeysState::isControlDown() const { return _bIsLeftControlDown || _bIsRightControlDown; }
bool ModifierKeysState::isLeftControlDown() const { return _bIsLeftControlDown; }
bool ModifierKeysState::isRightControlDown() const { return _bIsRightControlDown; }

bool ModifierKeysState::isAltDown() const { return _bIsLeftAltDown || _bIsRightAltDown; }
bool ModifierKeysState::isLeftAltDown() const { return _bIsLeftAltDown; }
bool ModifierKeysState::isRightAltDown() const { return _bIsRightAltDown; }

bool ModifierKeysState::AreModifersDown(ModifierKey ModiferKeys) const
{
    BEE_USE_MAGIC_ENUM_BIT_OPERATOR;

    bool AllModifersDown = true;
    if ((ModiferKeys & ModifierKey::Shift) == ModifierKey::Shift) {
        AllModifersDown &= isShiftDown();
    }
    if ((ModiferKeys & ModifierKey::Control) == ModifierKey::Control) {
        AllModifersDown &= isControlDown();
    }
    if ((ModiferKeys & ModifierKey::Alt) == ModifierKey::Alt) {
        AllModifersDown &= isAltDown();
    }

    return AllModifersDown;
}

bool ModifierKeysState::anyModifiersDown() const
{
    return isControlDown() || isShiftDown() || isAltDown();
}

String ModifierKeysState::toString() const
{
    if (!anyModifiersDown()) {
        return "Modifiers: -";
    }

    String str{"Modifiers:"};
    // clang-format off
    if (isLeftShiftDown())      str += " +LShift";
    if (isRightShiftDown())     str += " +RShift";
    if (isLeftControlDown())    str += " +LCtrl";
    if (isRightControlDown())   str += " +RCtrl";
    if (isLeftAltDown())        str += " +LAlt";
    if (isRightAltDown())       str += " +RAlt";
    // clang-format on

    return str;
}

String KeyboardEvent::toString() const
{
    return std::format("Event - Key: {}({}) [{}] {}", ToString(_key), ToString(_inputType), _modifierKeys.toString(), _bIsRepeat ? "+repeat" : "-");
}

MouseEvent::MouseEvent() : Event(EventType::Mouse), InputEvent(ModifierKeysState(), false, InputType::Unknown)
{
}

MouseEvent::MouseEvent(f32 x, f32 y, MouseButton btn, InputType mouseType, u8 clicks, const ModifierKeysState& modifierKeys, bool bIsRepeat)
    : Event(EventType::Mouse), InputEvent(modifierKeys, bIsRepeat, mouseType), _posX(x), _posY(y), _button(btn), _clicks(clicks)
{
}

MouseEvent::MouseEvent(f32 x, f32 y, f32 relX, f32 relY, const ModifierKeysState& modifierKeys)
    : Event(EventType::Mouse), InputEvent(modifierKeys, false, InputType::MouseMotion), _posX(x), _posY(y), _relX(relX), _relY(relY)
{
}

MouseEvent::MouseEvent(f32 h, f32 v, const ModifierKeysState& modifierKeys)
    : Event(EventType::Mouse), InputEvent(modifierKeys, false, InputType::MouseWheel), _posX(h), _posY(v)
{
}

Opt<MouseButton> MouseEvent::button() const
{
    if (isButtonEvent())
        return _button;

    return MouseButton::Unknown;
}

Opt<vec2f> MouseEvent::position() const
{
    if (_inputType == InputType::MouseMotion || isButtonEvent())
        return vec2f{_posX, _posY};

    return {};
}

Opt<vec2f> MouseEvent::relativeMotion() const
{
    if (_inputType == InputType::MouseMotion)
        return vec2f{_relX, _relY};

    return {};
}

Opt<vec2f> MouseEvent::wheelDelta() const
{
    if (_inputType == InputType::MouseWheel)
        return vec2f{_posX, _posY};

    return {};
}

Opt<u8> MouseEvent::clicks() const
{
    if (_inputType == InputType::MouseButtonDown)
        return _clicks;

    return {};
}

bool MouseEvent::isButtonEvent() const
{
    return _inputType == InputType::MouseButtonUp || _inputType == InputType::MouseButtonDown;
}

String MouseEvent::toString() const
{
    return std::format("Event - Mouse: {}({}) [{}] {}", ToString(_button), ToString(_inputType), _modifierKeys.toString(), _bIsRepeat ? "+repeat" : "-");
}