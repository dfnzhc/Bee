/**
 * @File InputEvents.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/10
 * @Brief This file is part of Bee.
 */

#include "Engine/Events/InputEvents.hpp"

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

bool ModifierKeysState::areModifersDown(ModifierKey ModiferKeys) const
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
    if (isLeftAltDown())        str += " +LA";
    if (isRightAltDown())       str += " +RA";
    if (isLeftControlDown())    str += " +LC";
    if (isRightControlDown())   str += " +RC";
    if (isLeftShiftDown())      str += " +LS";
    if (isRightShiftDown())     str += " +RS";
    // clang-format on

    return str;
}

String KeyPressedEvent::toString() const
{
    return std::format("KeyPressedEvent: [ Key: {}, Modifiers: {}, Repeat: {} ]", ToString(_key), _modifierKeys.toString(), _bIsRepeat ? "true" : "false");
}

String KeyReleasedEvent::toString() const
{
    return std::format("KeyReleasedEvent: [ Key: {}, Modifiers: {} ]", ToString(_key), _modifierKeys.toString());
}

String MouseButtonPressedEvent::toString() const
{
    return std::format("MouseButtonPressedEvent: [ Button: {}, "
                       "Pos: ({}, {}), "
                       "Clicks: {}, {}, "
                       "Repeat: {} ]",
                       ToString(_button),
                       _x,
                       _y,
                       _clicks,
                       _modifierKeys.toString(),
                       _bIsRepeat ? "true" : "false");
}

String MouseButtonReleasedEvent::toString() const
{
    return std::format("MouseButtonReleasedEvent: [ Button: {}, "
                       "Pos: ({}, {}), {} ]",
                       ToString(_button),
                       _x,
                       _y,
                       _modifierKeys.toString());
}

String MouseMovedEvent::toString() const
{
    return std::format("MouseButtonReleasedEvent: [ Pos: ({}, {}), {} ]",
                       _x,
                       _y,
                       _modifierKeys.toString());
}

String MouseWheelEvent::toString() const
{
    
    return std::format("MouseButtonReleasedEvent: [ Delta: ({}, {}), {} ]",
                       _deltaX,
                       _deltaY,
                       _modifierKeys.toString());
}
