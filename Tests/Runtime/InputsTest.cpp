/**
 * @File InputsTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <IO/Inputs.hpp>

using namespace bee;

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
    inputState.tick();
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
    inputState.tick();
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
    inputState.tick();
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
    inputState.tick();

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
    inputState.tick();

    // Release Shift in the next frame
    KeyboardEvent keyEventShiftUp;
    keyEventShiftUp.type = KeyboardEvent::Type::KeyReleased;
    keyEventShiftUp.key  = Key::LeftShift;
    inputState.onKeyEvent(keyEventShiftUp);
    EXPECT_TRUE(inputState.isModifierReleased(ModifierFlags::Shift));
}