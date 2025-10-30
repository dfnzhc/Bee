/**
 * @File InputManager.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/30
 * @Brief This file is part of Bee.
 */

#include "InputManager.hpp"
#include "../SDLHeader.hpp"

using namespace Bee;

VResult InputManager::initialize()
{
    if (_initialized)
    {
        return Ok();
    }

    _keyboardState.currentKeys.reset();
    _keyboardState.previousKeys.reset();
    _keyboardState.modifiers = KeyModifier::None;

    _mouseState.currentButtons.reset();
    _mouseState.previousButtons.reset();
    _mouseState.position      = {0, 0};
    _mouseState.deltaPosition = {0, 0};
    _mouseState.wheelDelta    = 0.0f;

    _initialized = true;
    return Ok();
}

void InputManager::shutdown()
{
    if (!_initialized)
    {
        return;
    }

    _initialized = false;
}

bool InputManager::isInitialized() const
{
    return _initialized;
}

void InputManager::processInputEvent(const InputEvent& event)
{
    try
    {
        std::visit([this](const auto& evt)
        {
            using T = std::decay_t<decltype(evt)>;

            if constexpr (std::is_same_v<T, KeyboardEvent>)
            {
                updateKeyboardState(evt);
            }
            else if constexpr (std::is_same_v<T, MouseMotionEvent>)
            {
                updateMouseState(evt);
            }
            else if constexpr (std::is_same_v<T, MouseButtonEvent>)
            {
                updateMouseState(evt);
            }
            else if constexpr (std::is_same_v<T, MouseWheelEvent>)
            {
                updateMouseState(evt);
            }
        }, event);
    }
    catch (const std::exception& e)
    {
        BEE_ERROR("事件执行错误: {}.", e.what());
    }
}

void InputManager::updateFrame()
{
    _keyboardState.previousKeys = _keyboardState.currentKeys;
    _mouseState.previousButtons = _mouseState.currentButtons;

    _mouseState.deltaPosition = {0, 0};
    _mouseState.wheelDelta    = 0.0f;
}

const KeyboardState& InputManager::getKeyboardState() const
{
    return _keyboardState;
}

bool InputManager::isKeyPressed(KeyCode key) const
{
    return isValidKeyCode(key) && _keyboardState.currentKeys[static_cast<Size>(key)];
}

bool InputManager::isKeyJustPressed(KeyCode key) const
{
    if (!isValidKeyCode(key))
        return false;

    const auto idx = static_cast<Size>(key);
    return _keyboardState.currentKeys[idx] && !_keyboardState.previousKeys[idx];
}

bool InputManager::isKeyJustReleased(KeyCode key) const
{
    if (!isValidKeyCode(key))
        return false;

    const auto idx = static_cast<Size>(key);
    return !_keyboardState.currentKeys[idx] && _keyboardState.previousKeys[idx];
}

KeyModifier InputManager::getKeyModifiers() const
{
    return _keyboardState.modifiers;
}

bool InputManager::hasKeyModifier(KeyModifier modifier) const
{
    return IsSet(_keyboardState.modifiers, modifier);
}

const MouseState& InputManager::getMouseState() const
{
    return _mouseState;
}

bool InputManager::isMouseButtonPressed(MouseButton button) const
{
    return isValidMouseButton(button) && _mouseState.currentButtons[static_cast<Size>(button)];
}

bool InputManager::isMouseButtonJustPressed(MouseButton button) const
{
    if (!isValidMouseButton(button))
    {
        return false;
    }

    const auto idx = static_cast<Size>(button);
    return _mouseState.currentButtons[idx] && !_mouseState.previousButtons[idx];
}

bool InputManager::isMouseButtonJustReleased(MouseButton button) const
{
    if (!isValidMouseButton(button))
    {
        return false;
    }

    const auto idx = static_cast<Size>(button);
    return !_mouseState.currentButtons[idx] && _mouseState.previousButtons[idx];
}

int2 InputManager::getMousePosition() const
{
    return _mouseState.position;
}

int2 InputManager::getMouseDelta() const
{
    return _mouseState.deltaPosition;
}

f32 InputManager::getMouseWheelDelta() const
{
    return _mouseState.wheelDelta;
}

void InputManager::updateKeyboardState(const KeyboardEvent& event)
{
    if (!isValidKeyCode(event.keyCode))
    {
        return;
    }

    _keyboardState.currentKeys[static_cast<Size>(event.keyCode)] = event.isPressed;

    _keyboardState.modifiers = event.modifiers;
}

void InputManager::updateMouseState(const MouseMotionEvent& event)
{
    _mouseState.deltaPosition.x = event.deltaX;
    _mouseState.deltaPosition.y = event.deltaY;
    _mouseState.position.x      = event.x;
    _mouseState.position.y      = event.y;
}

void InputManager::updateMouseState(const MouseButtonEvent& event)
{
    if (!isValidMouseButton(event.button))
    {
        return;
    }

    _mouseState.currentButtons[static_cast<Size>(event.button)] = event.isPressed;

    _mouseState.position.x = event.x;
    _mouseState.position.y = event.y;
}

void InputManager::updateMouseState(const MouseWheelEvent& event)
{
    _mouseState.wheelDelta = event.deltaY;
}

bool InputManager::isValidKeyCode(KeyCode key) const noexcept
{
    return static_cast<Size>(key) < _keyboardState.currentKeys.size();
}

bool InputManager::isValidMouseButton(MouseButton button) const noexcept
{
    return static_cast<Size>(button) < _mouseState.currentButtons.size();
}

bool InputManager::areKeysPressed(const std::vector<KeyCode>& keys) const
{
    return std::ranges::all_of(keys, [this](const auto& key)
    {
        return isKeyPressed(key);
    });
}

bool InputManager::isAnyKeyPressed() const
{
    return _keyboardState.currentKeys.any();
}

bool InputManager::isAnyMouseButtonPressed() const
{
    return _mouseState.currentButtons.any();
}

std::vector<KeyCode> InputManager::getPressedKeys() const
{
    std::vector<KeyCode> pressedKeys;

    
    for (Size i = 0; i < _keyboardState.currentKeys.size(); ++i)
    {
        if (_keyboardState.currentKeys[i])
        {
            pressedKeys.push_back(static_cast<KeyCode>(i));
        }
    }

    return pressedKeys;
}

std::vector<MouseButton> InputManager::getPressedMouseButtons() const
{
    std::vector<MouseButton> pressedButtons;

    for (Size i = 0; i < _mouseState.currentButtons.size(); ++i)
    {
        if (_mouseState.currentButtons[i])
        {
            pressedButtons.push_back(static_cast<MouseButton>(i));
        }
    }

    return pressedButtons;
}
