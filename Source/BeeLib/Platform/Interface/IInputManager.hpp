/**
 * @File IInputManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/29
 * @Brief This file is part of Bee.
 */

#pragma once

#include <bitset>
#include "PlatformTypes.hpp"

namespace Bee
{
    // -------------------- 
    // 输入错误
    struct InputErrors
    {
        static constexpr ErrorDomain kDomain = ErrorDomain::Input;

        static constexpr u16 kNotInitialized     = 0x0001;
        static constexpr u16 kInvalidKeyCode     = 0x0002;
        static constexpr u16 kInvalidMouseButton = 0x0003;
        static constexpr u16 kInternalFailure    = 0x0004;
    };

    constexpr auto MakeInputErr(u16 errCode)
    {
        return std::unexpected{Error{InputErrors::kDomain, errCode}};
    }

    // 键盘状态
    struct KeyboardState
    {
        static constexpr Size kMaxKeys = static_cast<Size>(KeyCode::Count);
        
        std::bitset<kMaxKeys> currentKeys;
        std::bitset<kMaxKeys> previousKeys;
        KeyModifier modifiers = KeyModifier::None;

        constexpr bool isKeyPressed(KeyCode key) const noexcept
        {
            auto idx = static_cast<Size>(key);
            return idx < currentKeys.size() && currentKeys[idx];
        }

        constexpr bool isKeyJustPressed(KeyCode key) const noexcept
        {
            auto idx = static_cast<Size>(key);
            return idx < currentKeys.size() && currentKeys[idx] && !previousKeys[idx];
        }

        constexpr bool isKeyJustReleased(KeyCode key) const noexcept
        {
            auto idx = static_cast<Size>(key);
            return idx < currentKeys.size() && !currentKeys[idx] && previousKeys[idx];
        }

        constexpr bool hasModifier(KeyModifier modifier) const noexcept
        {
            return (modifiers & modifier) != KeyModifier::None;
        }
    };

    // 鼠标状态
    struct MouseState
    {
        static constexpr Size kMaxMouseButtons = static_cast<Size>(MouseButton::Count);
        
        float2 position{0, 0};
        float2 deltaPosition{0, 0};
        f32 wheelDelta = 0.0f;

        std::bitset<kMaxMouseButtons> currentButtons;
        std::bitset<kMaxMouseButtons> previousButtons;

        constexpr bool isButtonPressed(MouseButton button) const noexcept
        {
            auto idx = static_cast<Size>(button);
            return idx < currentButtons.size() && currentButtons[idx];
        }

        constexpr bool isButtonJustPressed(MouseButton button) const noexcept
        {
            auto idx = static_cast<Size>(button);
            return idx < currentButtons.size() && currentButtons[idx] && !previousButtons[idx];
        }

        constexpr bool isButtonJustReleased(MouseButton button) const noexcept
        {
            auto idx = static_cast<Size>(button);
            return idx < currentButtons.size() && !currentButtons[idx] && previousButtons[idx];
        }
    };

    class IInputManager
    {
    public:
        virtual ~IInputManager() = default;

        // === 生命周期管理 ===
        virtual VResult initialize() = 0;
        virtual void shutdown() = 0;
        virtual bool isInitialized() const = 0;

        // === 事件输入及更新 ===
        virtual void processInputEvent(const InputEvent& event) = 0;
        virtual void updateFrame() = 0;

        // === 键盘输入查询 ===
        virtual const KeyboardState& getKeyboardState() const = 0;
        virtual bool isKeyPressed(KeyCode key) const = 0;
        virtual bool isKeyJustPressed(KeyCode key) const = 0;
        virtual bool isKeyJustReleased(KeyCode key) const = 0;
        virtual KeyModifier getKeyModifiers() const = 0;
        virtual bool hasKeyModifier(KeyModifier modifier) const = 0;

        // === 鼠标输入查询 ===
        virtual const MouseState& getMouseState() const = 0;
        virtual bool isMouseButtonPressed(MouseButton button) const = 0;
        virtual bool isMouseButtonJustPressed(MouseButton button) const = 0;
        virtual bool isMouseButtonJustReleased(MouseButton button) const = 0;

        virtual int2 getMousePosition() const = 0;
        virtual int2 getMouseDelta() const = 0;
        virtual f32 getMouseWheelDelta() const = 0;

        // === 其他功能 ===
        virtual bool areKeysPressed(const std::vector<KeyCode>& keys) const = 0;
        virtual bool isAnyKeyPressed() const = 0;
        virtual bool isAnyMouseButtonPressed() const = 0;
        virtual std::vector<KeyCode> getPressedKeys() const = 0;
        virtual std::vector<MouseButton> getPressedMouseButtons() const = 0;
    };
} // namespace Bee
