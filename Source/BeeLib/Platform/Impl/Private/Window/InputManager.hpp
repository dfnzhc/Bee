/**
 * @File InputManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include <vector>
#include "Platform/Interface/IInputManager.hpp"

namespace Bee
{
    class InputManager : public IInputManager
    {
    public:
        InputManager()           = default;
        ~InputManager() override = default;

        BEE_DISABLE_COPY_AND_MOVE(InputManager);

        // === 生命周期管理 ===
        VResult initialize() override;
        void shutdown() override;
        bool isInitialized() const override;

        // === 事件输入及更新 ===
        void processInputEvent(const InputEvent& event) override;
        void updateFrame() override;

        // === 键盘输入查询 ===
        const KeyboardState& getKeyboardState() const override;
        bool isKeyPressed(KeyCode key) const override;
        bool isKeyJustPressed(KeyCode key) const override;
        bool isKeyJustReleased(KeyCode key) const override;
        KeyModifier getKeyModifiers() const override;
        bool hasKeyModifier(KeyModifier modifier) const override;

        // === 鼠标输入查询 ===
        const MouseState& getMouseState() const override;
        bool isMouseButtonPressed(MouseButton button) const override;
        bool isMouseButtonJustPressed(MouseButton button) const override;
        bool isMouseButtonJustReleased(MouseButton button) const override;

        int2 getMousePosition() const override;
        int2 getMouseDelta() const override;
        f32 getMouseWheelDelta() const override;

        // === 其他功能 ===
        bool areKeysPressed(const std::vector<KeyCode>& keys) const override;
        bool isAnyKeyPressed() const override;
        bool isAnyMouseButtonPressed() const override;
        std::vector<KeyCode> getPressedKeys() const override;
        std::vector<MouseButton> getPressedMouseButtons() const override;

    private:
        // === 辅助方法 ===
        void updateKeyboardState(const KeyboardEvent& event);
        void updateMouseState(const MouseMotionEvent& event);
        void updateMouseState(const MouseButtonEvent& event);
        void updateMouseState(const MouseWheelEvent& event);

        bool isValidKeyCode(KeyCode key) const noexcept;
        bool isValidMouseButton(MouseButton button) const noexcept;

    private:
        KeyboardState _keyboardState{};
        MouseState _mouseState{};

        bool _initialized = false;
    };
} // namespace Bee
