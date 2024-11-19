/**
 * @File InputsTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <IO/Inputs.hpp>
#include <libassert/assert-gtest.hpp>

using namespace bee;

class MouseButtonEventTest : public ::testing::Test
{
protected:
    MouseEvent mouseEvent;
};

TEST_F(MouseButtonEventTest, HasModifierTest)
{
    // Test with no modifiers
    EXPECT_FALSE(mouseEvent.hasModifier(ModifierFlags::Shift));

    // Press Shift
    mouseEvent.mods.set(ModifierFlags::Shift);
    EXPECT_TRUE(mouseEvent.hasModifier(ModifierFlags::Shift));
    EXPECT_FALSE(mouseEvent.hasModifier(ModifierFlags::Ctrl));
    EXPECT_FALSE(mouseEvent.hasModifier(ModifierFlags::Alt));

    // Press Ctrl and Alt
    mouseEvent.mods.set(ModifierFlags::Ctrl);
    mouseEvent.mods.set(ModifierFlags::Alt);
    EXPECT_TRUE(mouseEvent.hasModifier(ModifierFlags::Shift));
    EXPECT_TRUE(mouseEvent.hasModifier(ModifierFlags::Ctrl));
    EXPECT_TRUE(mouseEvent.hasModifier(ModifierFlags::Alt));

    // Release Ctrl
    mouseEvent.mods.reset(ModifierFlags::Ctrl);
    EXPECT_TRUE(mouseEvent.hasModifier(ModifierFlags::Shift));
    EXPECT_FALSE(mouseEvent.hasModifier(ModifierFlags::Ctrl));
    EXPECT_TRUE(mouseEvent.hasModifier(ModifierFlags::Alt));

    // Release all modifiers
    mouseEvent.mods.reset(ModifierFlags::Shift);
    mouseEvent.mods.reset(ModifierFlags::Alt);
    EXPECT_FALSE(mouseEvent.hasModifier(ModifierFlags::Shift));
    EXPECT_FALSE(mouseEvent.hasModifier(ModifierFlags::Ctrl));
    EXPECT_FALSE(mouseEvent.hasModifier(ModifierFlags::Alt));
}

class KeyboardEventTest : public ::testing::Test
{
protected:
    KeyboardEvent keyEvent;
};

TEST_F(KeyboardEventTest, HasModifierTest)
{
    // Test with no modifiers
    EXPECT_FALSE(keyEvent.hasModifier(ModifierFlags::Shift));

    // Press Shift
    keyEvent.mods.set(ModifierFlags::Shift);
    EXPECT_TRUE(keyEvent.hasModifier(ModifierFlags::Shift));
    EXPECT_FALSE(keyEvent.hasModifier(ModifierFlags::Ctrl));
    EXPECT_FALSE(keyEvent.hasModifier(ModifierFlags::Alt));

    // Press Ctrl and Alt
    keyEvent.mods.set(ModifierFlags::Ctrl);
    keyEvent.mods.set(ModifierFlags::Alt);
    EXPECT_TRUE(keyEvent.hasModifier(ModifierFlags::Shift));
    EXPECT_TRUE(keyEvent.hasModifier(ModifierFlags::Ctrl));
    EXPECT_TRUE(keyEvent.hasModifier(ModifierFlags::Alt));

    // Release Ctrl
    keyEvent.mods.reset(ModifierFlags::Ctrl);
    EXPECT_TRUE(keyEvent.hasModifier(ModifierFlags::Shift));
    EXPECT_FALSE(keyEvent.hasModifier(ModifierFlags::Ctrl));
    EXPECT_TRUE(keyEvent.hasModifier(ModifierFlags::Alt));

    // Release all modifiers
    keyEvent.mods.reset(ModifierFlags::Shift);
    keyEvent.mods.reset(ModifierFlags::Alt);
    EXPECT_FALSE(keyEvent.hasModifier(ModifierFlags::Shift));
    EXPECT_FALSE(keyEvent.hasModifier(ModifierFlags::Ctrl));
    EXPECT_FALSE(keyEvent.hasModifier(ModifierFlags::Alt));
}

class InputStateTest : public ::testing::Test
{
protected:
    InputState inputState;
};

TEST_F(InputStateTest, MouseMovingTest)
{
    MouseEvent mouseEvent;
    mouseEvent.type = MouseEvent::Type::Move;
    inputState.onMouseEvent(mouseEvent);
    EXPECT_TRUE(inputState.isMouseMoving());
    inputState.endFrame();
    EXPECT_FALSE(inputState.isMouseMoving());
}

TEST_F(InputStateTest, MouseButtonDownTest)
{
    MouseEvent mouseEvent;
    mouseEvent.type   = MouseEvent::Type::ButtonDown;
    mouseEvent.button = MouseButton::Left;
    inputState.onMouseEvent(mouseEvent);
    EXPECT_TRUE(inputState.isMouseButtonDown(MouseButton::Left));
    EXPECT_TRUE(inputState.isMouseButtonClicked(MouseButton::Left));
    EXPECT_FALSE(inputState.isMouseButtonReleased(MouseButton::Left));
}

TEST_F(InputStateTest, MouseButtonUpTest)
{
    MouseEvent mouseEvent;
    mouseEvent.button = MouseButton::Left;
    mouseEvent.type   = MouseEvent::Type::ButtonDown;
    inputState.onMouseEvent(mouseEvent);
    inputState.endFrame();
    mouseEvent.type = MouseEvent::Type::ButtonUp;
    inputState.onMouseEvent(mouseEvent);
    EXPECT_FALSE(inputState.isMouseButtonDown(MouseButton::Left));
    EXPECT_FALSE(inputState.isMouseButtonClicked(MouseButton::Left));
    EXPECT_TRUE(inputState.isMouseButtonReleased(MouseButton::Left));
}

TEST_F(InputStateTest, KeyDownTest)
{
    KeyboardEvent keyEvent;
    keyEvent.type = KeyboardEvent::Type::KeyPressed;
    keyEvent.key  = Key::A;
    inputState.onKeyEvent(keyEvent);
    EXPECT_TRUE(inputState.isKeyDown(Key::A));
    EXPECT_TRUE(inputState.isKeyPressed(Key::A));
    EXPECT_FALSE(inputState.isKeyReleased(Key::A));
}

TEST_F(InputStateTest, KeyUpTest)
{
    KeyboardEvent keyEvent;
    keyEvent.key  = Key::A;
    keyEvent.type = KeyboardEvent::Type::KeyPressed;
    inputState.onKeyEvent(keyEvent);
    inputState.endFrame();
    keyEvent.type = KeyboardEvent::Type::KeyReleased;
    inputState.onKeyEvent(keyEvent);
    EXPECT_FALSE(inputState.isKeyDown(Key::A));
    EXPECT_FALSE(inputState.isKeyPressed(Key::A));
    EXPECT_TRUE(inputState.isKeyReleased(Key::A));
}

TEST_F(InputStateTest, ModifierDownTest)
{
    KeyboardEvent keyEvent;
    keyEvent.type = KeyboardEvent::Type::KeyPressed;
    keyEvent.key  = Key::LeftShift;
    inputState.onKeyEvent(keyEvent);
    EXPECT_TRUE(inputState.isModifierDown(ModifierFlags::Shift));
}

TEST_F(InputStateTest, ModifierUpTest)
{
    KeyboardEvent keyEvent;
    keyEvent.type = KeyboardEvent::Type::KeyReleased;
    keyEvent.key  = Key::LeftShift;
    inputState.onKeyEvent(keyEvent);
    EXPECT_FALSE(inputState.isModifierDown(ModifierFlags::Shift));
}

TEST_F(InputStateTest, ModifierPressedTest)
{
    // Press Shift
    KeyboardEvent keyEventShiftDown;
    keyEventShiftDown.type = KeyboardEvent::Type::KeyPressed;
    keyEventShiftDown.key  = Key::LeftShift;
    inputState.onKeyEvent(keyEventShiftDown);
    EXPECT_TRUE(inputState.isModifierPressed(ModifierFlags::Shift));
    inputState.endFrame();

    // Release Shift
    KeyboardEvent keyEventShiftUp;
    keyEventShiftUp.type = KeyboardEvent::Type::KeyReleased;
    keyEventShiftUp.key  = Key::LeftShift;
    inputState.onKeyEvent(keyEventShiftUp);
    EXPECT_FALSE(inputState.isModifierPressed(ModifierFlags::Shift));
}

TEST_F(InputStateTest, ModifierReleasedTest)
{
    // Press Shift
    KeyboardEvent keyEventShiftDown;
    keyEventShiftDown.type = KeyboardEvent::Type::KeyPressed;
    keyEventShiftDown.key  = Key::LeftShift;
    inputState.onKeyEvent(keyEventShiftDown);
    inputState.endFrame();

    // Release Shift in the next frame
    KeyboardEvent keyEventShiftUp;
    keyEventShiftUp.type = KeyboardEvent::Type::KeyReleased;
    keyEventShiftUp.key  = Key::LeftShift;
    inputState.onKeyEvent(keyEventShiftUp);
    EXPECT_TRUE(inputState.isModifierReleased(ModifierFlags::Shift));
}