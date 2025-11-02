/**
 * @File InputManagerTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/30
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <format>
#include "../../../Source/BeeLib/Platform/Impl/Private/InputManager.hpp"
#include "Platform/Interface/PlatformTypes.hpp"

using namespace Bee;
using namespace testing;

class InputManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        inputManager = std::make_unique<InputManager>();
    }

    void TearDown() override
    {
        if (inputManager && inputManager->isInitialized())
        {
            inputManager->shutdown();
        }
        inputManager.reset();
    }

    InputEvent createKeyboardEvent(PlatformEventType type, KeyCode keyCode, KeyModifier modifiers = KeyModifier::None)
    {
        KeyboardEvent keyEvent{};
        keyEvent.type      = type;
        keyEvent.keyCode   = keyCode;
        keyEvent.modifiers = modifiers;
        keyEvent.isRepeat  = false;
        keyEvent.isPressed = (type == PlatformEventType::KeyDown);

        return InputEvent{keyEvent};
    }

    InputEvent createMouseButtonEvent(PlatformEventType type, MouseButton button, f32 x = 0.0f, f32 y = 0.0f)
    {
        MouseButtonEvent mouseEvent{};
        mouseEvent.type      = type;
        mouseEvent.button    = button;
        mouseEvent.x         = x;
        mouseEvent.y         = y;
        mouseEvent.isPressed = (type == PlatformEventType::MouseButtonDown);

        return InputEvent{mouseEvent};
    }

    InputEvent createMouseMoveEvent(f32 x, f32 y, f32 deltaX = 0.0f, f32 deltaY = 0.0f)
    {
        MouseMotionEvent moveEvent{};
        moveEvent.type   = PlatformEventType::MouseMotion;
        moveEvent.x      = x;
        moveEvent.y      = y;
        moveEvent.deltaX = deltaX;
        moveEvent.deltaY = deltaY;

        return InputEvent{moveEvent};
    }

    InputEvent createMouseWheelEvent(f32 delta, f32 x = 0.0f, f32 y = 0.0f)
    {
        MouseWheelEvent wheelEvent{};
        wheelEvent.type   = PlatformEventType::MouseWheel;
        wheelEvent.deltaY = delta;
        wheelEvent.mouseX = x;
        wheelEvent.mouseY = y;

        return InputEvent{wheelEvent};
    }

    std::unique_ptr<InputManager> inputManager;
};

TEST_F(InputManagerTest, InitializeAndShutdown)
{
    EXPECT_FALSE(inputManager->isInitialized());

    auto result = inputManager->initialize();
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(inputManager->isInitialized());

    result = inputManager->initialize();
    EXPECT_TRUE(result.has_value());

    inputManager->shutdown();
    EXPECT_FALSE(inputManager->isInitialized());

    inputManager->shutdown();
    EXPECT_FALSE(inputManager->isInitialized());
}

TEST_F(InputManagerTest, OperationsRequireInitialization)
{
    EXPECT_FALSE(inputManager->isKeyPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustReleased(KeyCode::A));
    EXPECT_FALSE(inputManager->isMouseButtonPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustReleased(MouseButton::Left));

    auto mousePos = inputManager->getMousePosition();
    EXPECT_EQ(mousePos.x, 0);
    EXPECT_EQ(mousePos.y, 0);

    auto mouseDelta = inputManager->getMouseDelta();
    EXPECT_EQ(mouseDelta.x, 0);
    EXPECT_EQ(mouseDelta.y, 0);

    EXPECT_EQ(inputManager->getMouseWheelDelta(), 0.0f);
    EXPECT_EQ(inputManager->getKeyModifiers(), KeyModifier::None);
    EXPECT_FALSE(inputManager->hasKeyModifier(KeyModifier::Ctrl));
    EXPECT_FALSE(inputManager->isAnyKeyPressed());
    EXPECT_FALSE(inputManager->isAnyMouseButtonPressed());

    auto pressedKeys = inputManager->getPressedKeys();
    EXPECT_TRUE(pressedKeys.empty());

    auto pressedButtons = inputManager->getPressedMouseButtons();
    EXPECT_TRUE(pressedButtons.empty());
}

TEST_F(InputManagerTest, KeyboardBasicInput)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    EXPECT_FALSE(inputManager->isKeyPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustReleased(KeyCode::A));

    auto keyDownEvent = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::A);
    inputManager->processInputEvent(keyDownEvent);

    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::A));
    EXPECT_TRUE(inputManager->isKeyJustPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustReleased(KeyCode::A));

    inputManager->updateFrame();
    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustReleased(KeyCode::A));

    auto keyUpEvent = createKeyboardEvent(PlatformEventType::KeyUp, KeyCode::A);
    inputManager->processInputEvent(keyUpEvent);

    EXPECT_FALSE(inputManager->isKeyPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustPressed(KeyCode::A));
    EXPECT_TRUE(inputManager->isKeyJustReleased(KeyCode::A));

    inputManager->updateFrame();
    EXPECT_FALSE(inputManager->isKeyPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustPressed(KeyCode::A));
    EXPECT_FALSE(inputManager->isKeyJustReleased(KeyCode::A));
}

TEST_F(InputManagerTest, KeyboardModifiers)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    auto ctrlKeyEvent = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::A, KeyModifier::Ctrl);
    inputManager->processInputEvent(ctrlKeyEvent);

    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::A));
    EXPECT_EQ(inputManager->getKeyModifiers(), KeyModifier::Ctrl);
    EXPECT_TRUE(inputManager->hasKeyModifier(KeyModifier::Ctrl));
    EXPECT_FALSE(inputManager->hasKeyModifier(KeyModifier::Shift));

    auto ctrlShiftEvent = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::B,
                                              KeyModifier::Ctrl | KeyModifier::Shift);
    inputManager->processInputEvent(ctrlShiftEvent);

    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::B));
    EXPECT_EQ(inputManager->getKeyModifiers(), KeyModifier::Ctrl | KeyModifier::Shift);
    EXPECT_TRUE(inputManager->hasKeyModifier(KeyModifier::Ctrl));
    EXPECT_TRUE(inputManager->hasKeyModifier(KeyModifier::Shift));
    EXPECT_FALSE(inputManager->hasKeyModifier(KeyModifier::Alt));
}

TEST_F(InputManagerTest, MultipleKeysPressed)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    auto keyA = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::A);
    auto keyB = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::B);
    auto keyC = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::C);

    inputManager->processInputEvent(keyA);
    inputManager->processInputEvent(keyB);
    inputManager->processInputEvent(keyC);

    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::A));
    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::B));
    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::C));

    std::vector<KeyCode> testKeys = {KeyCode::A, KeyCode::B, KeyCode::C};
    EXPECT_TRUE(inputManager->areKeysPressed(testKeys));

    std::vector<KeyCode> partialKeys = {KeyCode::A, KeyCode::D};
    EXPECT_FALSE(inputManager->areKeysPressed(partialKeys));

    EXPECT_TRUE(inputManager->isAnyKeyPressed());

    auto pressedKeys = inputManager->getPressedKeys();
    EXPECT_EQ(pressedKeys.size(), 3);
    EXPECT_THAT(pressedKeys, UnorderedElementsAre(KeyCode::A, KeyCode::B, KeyCode::C));
}

TEST_F(InputManagerTest, MouseButtonInput)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    EXPECT_FALSE(inputManager->isMouseButtonPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustReleased(MouseButton::Left));

    auto leftDownEvent = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Left, 100.0f, 200.0f);
    inputManager->processInputEvent(leftDownEvent);

    EXPECT_TRUE(inputManager->isMouseButtonPressed(MouseButton::Left));
    EXPECT_TRUE(inputManager->isMouseButtonJustPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustReleased(MouseButton::Left));

    inputManager->updateFrame();
    EXPECT_TRUE(inputManager->isMouseButtonPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustReleased(MouseButton::Left));

    auto leftUpEvent = createMouseButtonEvent(PlatformEventType::MouseButtonUp, MouseButton::Left, 100.0f, 200.0f);
    inputManager->processInputEvent(leftUpEvent);

    EXPECT_FALSE(inputManager->isMouseButtonPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustPressed(MouseButton::Left));
    EXPECT_TRUE(inputManager->isMouseButtonJustReleased(MouseButton::Left));

    inputManager->updateFrame();
    EXPECT_FALSE(inputManager->isMouseButtonPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustPressed(MouseButton::Left));
    EXPECT_FALSE(inputManager->isMouseButtonJustReleased(MouseButton::Left));
}

TEST_F(InputManagerTest, MousePosition)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    auto pos = inputManager->getMousePosition();
    EXPECT_EQ(pos.x, 0);
    EXPECT_EQ(pos.y, 0);

    auto delta = inputManager->getMouseDelta();
    EXPECT_EQ(delta.x, 0);
    EXPECT_EQ(delta.y, 0);

    auto moveEvent = createMouseMoveEvent(150.0f, 250.0f, 10.0f, 15.0f);
    inputManager->processInputEvent(moveEvent);

    pos = inputManager->getMousePosition();
    EXPECT_EQ(pos.x, 150);
    EXPECT_EQ(pos.y, 250);

    delta = inputManager->getMouseDelta();
    EXPECT_EQ(delta.x, 10);
    EXPECT_EQ(delta.y, 15);

    inputManager->updateFrame();
    delta = inputManager->getMouseDelta();
    EXPECT_EQ(delta.x, 0);
    EXPECT_EQ(delta.y, 0);

    pos = inputManager->getMousePosition();
    EXPECT_EQ(pos.x, 150);
    EXPECT_EQ(pos.y, 250);
}

TEST_F(InputManagerTest, MouseWheel)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    EXPECT_EQ(inputManager->getMouseWheelDelta(), 0.0f);

    auto wheelEvent = createMouseWheelEvent(1.5f, 100.0f, 100.0f);
    inputManager->processInputEvent(wheelEvent);

    EXPECT_FLOAT_EQ(inputManager->getMouseWheelDelta(), 1.5f);

    inputManager->updateFrame();
    EXPECT_EQ(inputManager->getMouseWheelDelta(), 0.0f);

    wheelEvent = createMouseWheelEvent(-2.0f, 100.0f, 100.0f);
    inputManager->processInputEvent(wheelEvent);

    EXPECT_FLOAT_EQ(inputManager->getMouseWheelDelta(), -2.0f);
}

TEST_F(InputManagerTest, MultipleMouseButtons)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    auto leftDown   = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Left);
    auto rightDown  = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Right);
    auto middleDown = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Middle);

    inputManager->processInputEvent(leftDown);
    inputManager->processInputEvent(rightDown);
    inputManager->processInputEvent(middleDown);

    EXPECT_TRUE(inputManager->isMouseButtonPressed(MouseButton::Left));
    EXPECT_TRUE(inputManager->isMouseButtonPressed(MouseButton::Right));
    EXPECT_TRUE(inputManager->isMouseButtonPressed(MouseButton::Middle));

    EXPECT_TRUE(inputManager->isAnyMouseButtonPressed());

    auto pressedButtons = inputManager->getPressedMouseButtons();
    EXPECT_EQ(pressedButtons.size(), 3);
    EXPECT_THAT(pressedButtons, UnorderedElementsAre(MouseButton::Left, MouseButton::Right, MouseButton::Middle));
}

TEST_F(InputManagerTest, KeyboardStateAccess)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    auto keyA = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::A, KeyModifier::Ctrl);
    inputManager->processInputEvent(keyA);

    const auto& keyboardState = inputManager->getKeyboardState();

    EXPECT_TRUE(keyboardState.isKeyPressed(KeyCode::A));
    EXPECT_TRUE(keyboardState.isKeyJustPressed(KeyCode::A));
    EXPECT_FALSE(keyboardState.isKeyJustReleased(KeyCode::A));
    EXPECT_TRUE(keyboardState.hasModifier(KeyModifier::Ctrl));
    EXPECT_FALSE(keyboardState.hasModifier(KeyModifier::Shift));
}

TEST_F(InputManagerTest, MouseStateAccess)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    auto moveEvent   = createMouseMoveEvent(100.0f, 200.0f, 5.0f, 10.0f);
    auto buttonEvent = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Left, 100.0f, 200.0f);
    auto wheelEvent  = createMouseWheelEvent(1.0f, 100.0f, 200.0f);

    inputManager->processInputEvent(moveEvent);
    inputManager->processInputEvent(buttonEvent);
    inputManager->processInputEvent(wheelEvent);

    const auto& mouseState = inputManager->getMouseState();

    EXPECT_EQ(mouseState.position.x, 100.0f);
    EXPECT_EQ(mouseState.position.y, 200.0f);
    EXPECT_EQ(mouseState.deltaPosition.x, 5.0f);
    EXPECT_EQ(mouseState.deltaPosition.y, 10.0f);
    EXPECT_EQ(mouseState.wheelDelta, 1.0f);
    EXPECT_TRUE(mouseState.isButtonPressed(MouseButton::Left));
    EXPECT_TRUE(mouseState.isButtonJustPressed(MouseButton::Left));
    EXPECT_FALSE(mouseState.isButtonJustReleased(MouseButton::Left));
}

TEST_F(InputManagerTest, InvalidKeyCodeHandling)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    EXPECT_FALSE(inputManager->isKeyPressed(static_cast<KeyCode>(999)));
    EXPECT_FALSE(inputManager->isKeyJustPressed(static_cast<KeyCode>(999)));
    EXPECT_FALSE(inputManager->isKeyJustReleased(static_cast<KeyCode>(999)));
}

TEST_F(InputManagerTest, InvalidMouseButtonHandling)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    EXPECT_FALSE(inputManager->isMouseButtonPressed(static_cast<MouseButton>(999)));
    EXPECT_FALSE(inputManager->isMouseButtonJustPressed(static_cast<MouseButton>(999)));
    EXPECT_FALSE(inputManager->isMouseButtonJustReleased(static_cast<MouseButton>(999)));
}

TEST_F(InputManagerTest, EmptyKeyListHandling)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    std::vector<KeyCode> emptyKeys;
    EXPECT_TRUE(inputManager->areKeysPressed(emptyKeys));
}

TEST_F(InputManagerTest, FrameUpdateConsistency)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    auto keyEvent = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::Space);
    inputManager->processInputEvent(keyEvent);

    EXPECT_TRUE(inputManager->isKeyJustPressed(KeyCode::Space));
    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::Space));

    inputManager->updateFrame();

    EXPECT_FALSE(inputManager->isKeyJustPressed(KeyCode::Space));
    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::Space));

    inputManager->updateFrame();

    EXPECT_FALSE(inputManager->isKeyJustPressed(KeyCode::Space));
    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::Space));
}

TEST_F(InputManagerTest, ComplexInputSequence)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    // 1. 按下 Ctrl+A
    auto ctrlA = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::A, KeyModifier::Ctrl);
    inputManager->processInputEvent(ctrlA);

    // 2. 移动鼠标
    auto mouseMove = createMouseMoveEvent(50.0f, 75.0f, 10.0f, 15.0f);
    inputManager->processInputEvent(mouseMove);

    // 3. 按下鼠标左键
    auto mouseDown = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Left, 50.0f, 75.0f);
    inputManager->processInputEvent(mouseDown);

    // 4. 滚动鼠标滚轮
    auto mouseWheel = createMouseWheelEvent(2.5f, 50.0f, 75.0f);
    inputManager->processInputEvent(mouseWheel);

    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::A));
    EXPECT_TRUE(inputManager->hasKeyModifier(KeyModifier::Ctrl));
    EXPECT_TRUE(inputManager->isMouseButtonPressed(MouseButton::Left));

    auto mousePos = inputManager->getMousePosition();
    EXPECT_EQ(mousePos.x, 50);
    EXPECT_EQ(mousePos.y, 75);

    auto mouseDelta = inputManager->getMouseDelta();
    EXPECT_EQ(mouseDelta.x, 10);
    EXPECT_EQ(mouseDelta.y, 15);

    EXPECT_FLOAT_EQ(inputManager->getMouseWheelDelta(), 2.5f);

    // 更新帧
    inputManager->updateFrame();

    // 验证帧更新后的状态
    EXPECT_TRUE(inputManager->isKeyPressed(KeyCode::A));                     // 仍然按下
    EXPECT_FALSE(inputManager->isKeyJustPressed(KeyCode::A));                // 不再是刚按下
    EXPECT_TRUE(inputManager->isMouseButtonPressed(MouseButton::Left));      // 仍然按下
    EXPECT_FALSE(inputManager->isMouseButtonJustPressed(MouseButton::Left)); // 不再是刚按下

    // Delta 值应该被清零
    mouseDelta = inputManager->getMouseDelta();
    EXPECT_EQ(mouseDelta.x, 0);
    EXPECT_EQ(mouseDelta.y, 0);
    EXPECT_EQ(inputManager->getMouseWheelDelta(), 0.0f);
}

TEST_F(InputManagerTest, RapidInputEvents)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    for (int i = 0; i < 10; ++i)
    {
        auto keyDown = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::Space);
        auto keyUp   = createKeyboardEvent(PlatformEventType::KeyUp, KeyCode::Space);

        inputManager->processInputEvent(keyDown);
        inputManager->updateFrame();
        inputManager->processInputEvent(keyUp);
    }

    EXPECT_FALSE(inputManager->isKeyPressed(KeyCode::Space));
    EXPECT_FALSE(inputManager->isKeyJustPressed(KeyCode::Space));
    EXPECT_TRUE(inputManager->isKeyJustReleased(KeyCode::Space));
}

TEST_F(InputManagerTest, KeyEventCallback)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    std::vector<KeyboardEvent> capturedEvents;
    inputManager->setKeyEventCallback([&capturedEvents](const KeyboardEvent& event)
    {
        capturedEvents.push_back(event);
    });

    auto keyDownEvent = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::A, KeyModifier::Ctrl);
    inputManager->processInputEvent(keyDownEvent);

    ASSERT_EQ(capturedEvents.size(), 1);
    EXPECT_EQ(capturedEvents[0].type, PlatformEventType::KeyDown);
    EXPECT_EQ(capturedEvents[0].keyCode, KeyCode::A);
    EXPECT_EQ(capturedEvents[0].modifiers, KeyModifier::Ctrl);
    EXPECT_TRUE(capturedEvents[0].isPressed);

    auto keyUpEvent = createKeyboardEvent(PlatformEventType::KeyUp, KeyCode::A, KeyModifier::None);
    inputManager->processInputEvent(keyUpEvent);

    ASSERT_EQ(capturedEvents.size(), 2);
    EXPECT_EQ(capturedEvents[1].type, PlatformEventType::KeyUp);
    EXPECT_EQ(capturedEvents[1].keyCode, KeyCode::A);
    EXPECT_EQ(capturedEvents[1].modifiers, KeyModifier::None);
    EXPECT_FALSE(capturedEvents[1].isPressed);

    auto keyBEvent = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::B, KeyModifier::Shift);
    inputManager->processInputEvent(keyBEvent);

    ASSERT_EQ(capturedEvents.size(), 3);
    EXPECT_EQ(capturedEvents[2].type, PlatformEventType::KeyDown);
    EXPECT_EQ(capturedEvents[2].keyCode, KeyCode::B);
    EXPECT_EQ(capturedEvents[2].modifiers, KeyModifier::Shift);
    EXPECT_TRUE(capturedEvents[2].isPressed);
}

TEST_F(InputManagerTest, MouseButtonCallback)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    std::vector<MouseButtonEvent> capturedEvents;
    inputManager->setMouseButtonCallback([&capturedEvents](const MouseButtonEvent& event)
    {
        capturedEvents.push_back(event);
    });

    auto leftDownEvent = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Left, 100.0f, 200.0f);
    inputManager->processInputEvent(leftDownEvent);

    ASSERT_EQ(capturedEvents.size(), 1);
    EXPECT_EQ(capturedEvents[0].type, PlatformEventType::MouseButtonDown);
    EXPECT_EQ(capturedEvents[0].button, MouseButton::Left);
    EXPECT_FLOAT_EQ(capturedEvents[0].x, 100.0f);
    EXPECT_FLOAT_EQ(capturedEvents[0].y, 200.0f);
    EXPECT_TRUE(capturedEvents[0].isPressed);

    auto leftUpEvent = createMouseButtonEvent(PlatformEventType::MouseButtonUp, MouseButton::Left, 100.0f, 200.0f);
    inputManager->processInputEvent(leftUpEvent);

    ASSERT_EQ(capturedEvents.size(), 2);
    EXPECT_EQ(capturedEvents[1].type, PlatformEventType::MouseButtonUp);
    EXPECT_EQ(capturedEvents[1].button, MouseButton::Left);
    EXPECT_FLOAT_EQ(capturedEvents[1].x, 100.0f);
    EXPECT_FLOAT_EQ(capturedEvents[1].y, 200.0f);
    EXPECT_FALSE(capturedEvents[1].isPressed);

    auto rightDownEvent = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Right, 150.0f, 250.0f);
    inputManager->processInputEvent(rightDownEvent);

    ASSERT_EQ(capturedEvents.size(), 3);
    EXPECT_EQ(capturedEvents[2].type, PlatformEventType::MouseButtonDown);
    EXPECT_EQ(capturedEvents[2].button, MouseButton::Right);
    EXPECT_FLOAT_EQ(capturedEvents[2].x, 150.0f);
    EXPECT_FLOAT_EQ(capturedEvents[2].y, 250.0f);
    EXPECT_TRUE(capturedEvents[2].isPressed);
}

TEST_F(InputManagerTest, MouseMotionCallback)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    // 设置回调函数来捕获鼠标移动事件
    std::vector<MouseMotionEvent> capturedEvents;
    inputManager->setMouseMotionCallback([&capturedEvents](const MouseMotionEvent& event)
    {
        capturedEvents.push_back(event);
    });

    // 测试鼠标移动事件
    auto moveEvent1 = createMouseMoveEvent(100.0f, 200.0f, 10.0f, 20.0f);
    inputManager->processInputEvent(moveEvent1);

    ASSERT_EQ(capturedEvents.size(), 1);
    EXPECT_EQ(capturedEvents[0].type, PlatformEventType::MouseMotion);
    EXPECT_FLOAT_EQ(capturedEvents[0].x, 100.0f);
    EXPECT_FLOAT_EQ(capturedEvents[0].y, 200.0f);
    EXPECT_FLOAT_EQ(capturedEvents[0].deltaX, 10.0f);
    EXPECT_FLOAT_EQ(capturedEvents[0].deltaY, 20.0f);

    // 测试另一个鼠标移动事件
    auto moveEvent2 = createMouseMoveEvent(150.0f, 250.0f, -5.0f, 15.0f);
    inputManager->processInputEvent(moveEvent2);

    ASSERT_EQ(capturedEvents.size(), 2);
    EXPECT_EQ(capturedEvents[1].type, PlatformEventType::MouseMotion);
    EXPECT_FLOAT_EQ(capturedEvents[1].x, 150.0f);
    EXPECT_FLOAT_EQ(capturedEvents[1].y, 250.0f);
    EXPECT_FLOAT_EQ(capturedEvents[1].deltaX, -5.0f);
    EXPECT_FLOAT_EQ(capturedEvents[1].deltaY, 15.0f);

    // 测试零增量的鼠标移动事件
    auto moveEvent3 = createMouseMoveEvent(150.0f, 250.0f, 0.0f, 0.0f);
    inputManager->processInputEvent(moveEvent3);

    ASSERT_EQ(capturedEvents.size(), 3);
    EXPECT_EQ(capturedEvents[2].type, PlatformEventType::MouseMotion);
    EXPECT_FLOAT_EQ(capturedEvents[2].x, 150.0f);
    EXPECT_FLOAT_EQ(capturedEvents[2].y, 250.0f);
    EXPECT_FLOAT_EQ(capturedEvents[2].deltaX, 0.0f);
    EXPECT_FLOAT_EQ(capturedEvents[2].deltaY, 0.0f);
}

TEST_F(InputManagerTest, MouseWheelCallback)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    // 设置回调函数来捕获鼠标滚轮事件
    std::vector<MouseWheelEvent> capturedEvents;
    inputManager->setMouseWheelCallback([&capturedEvents](const MouseWheelEvent& event)
    {
        capturedEvents.push_back(event);
    });

    // 测试向上滚动事件
    auto wheelUpEvent = createMouseWheelEvent(1.5f, 100.0f, 200.0f);
    inputManager->processInputEvent(wheelUpEvent);

    ASSERT_EQ(capturedEvents.size(), 1);
    EXPECT_EQ(capturedEvents[0].type, PlatformEventType::MouseWheel);
    EXPECT_FLOAT_EQ(capturedEvents[0].deltaY, 1.5f);
    EXPECT_FLOAT_EQ(capturedEvents[0].mouseX, 100.0f);
    EXPECT_FLOAT_EQ(capturedEvents[0].mouseY, 200.0f);

    // 测试向下滚动事件
    auto wheelDownEvent = createMouseWheelEvent(-2.0f, 150.0f, 250.0f);
    inputManager->processInputEvent(wheelDownEvent);

    ASSERT_EQ(capturedEvents.size(), 2);
    EXPECT_EQ(capturedEvents[1].type, PlatformEventType::MouseWheel);
    EXPECT_FLOAT_EQ(capturedEvents[1].deltaY, -2.0f);
    EXPECT_FLOAT_EQ(capturedEvents[1].mouseX, 150.0f);
    EXPECT_FLOAT_EQ(capturedEvents[1].mouseY, 250.0f);

    // 测试零滚动事件
    auto wheelZeroEvent = createMouseWheelEvent(0.0f, 200.0f, 300.0f);
    inputManager->processInputEvent(wheelZeroEvent);

    ASSERT_EQ(capturedEvents.size(), 3);
    EXPECT_EQ(capturedEvents[2].type, PlatformEventType::MouseWheel);
    EXPECT_FLOAT_EQ(capturedEvents[2].deltaY, 0.0f);
    EXPECT_FLOAT_EQ(capturedEvents[2].mouseX, 200.0f);
    EXPECT_FLOAT_EQ(capturedEvents[2].mouseY, 300.0f);
}

TEST_F(InputManagerTest, ClearEventCallbacks)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    // 设置所有类型的回调函数
    std::vector<KeyboardEvent> keyEvents;
    std::vector<MouseButtonEvent> buttonEvents;
    std::vector<MouseMotionEvent> motionEvents;
    std::vector<MouseWheelEvent> wheelEvents;

    inputManager->setKeyEventCallback([&keyEvents](const KeyboardEvent& event)
    {
        keyEvents.push_back(event);
    });
    inputManager->setMouseButtonCallback([&buttonEvents](const MouseButtonEvent& event)
    {
        buttonEvents.push_back(event);
    });
    inputManager->setMouseMotionCallback([&motionEvents](const MouseMotionEvent& event)
    {
        motionEvents.push_back(event);
    });
    inputManager->setMouseWheelCallback([&wheelEvents](const MouseWheelEvent& event)
    {
        wheelEvents.push_back(event);
    });

    // 发送各种事件，验证回调被调用
    auto keyEvent    = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::A);
    auto buttonEvent = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Left, 100.0f, 200.0f);
    auto motionEvent = createMouseMoveEvent(150.0f, 250.0f, 10.0f, 20.0f);
    auto wheelEvent  = createMouseWheelEvent(1.0f, 100.0f, 200.0f);

    inputManager->processInputEvent(keyEvent);
    inputManager->processInputEvent(buttonEvent);
    inputManager->processInputEvent(motionEvent);
    inputManager->processInputEvent(wheelEvent);

    // 验证所有回调都被调用了
    EXPECT_EQ(keyEvents.size(), 1);
    EXPECT_EQ(buttonEvents.size(), 1);
    EXPECT_EQ(motionEvents.size(), 1);
    EXPECT_EQ(wheelEvents.size(), 1);

    // 清除所有回调
    inputManager->clearEventCallbacks();

    // 再次发送相同的事件
    inputManager->processInputEvent(keyEvent);
    inputManager->processInputEvent(buttonEvent);
    inputManager->processInputEvent(motionEvent);
    inputManager->processInputEvent(wheelEvent);

    // 验证回调没有被调用（事件数量没有增加）
    EXPECT_EQ(keyEvents.size(), 1);
    EXPECT_EQ(buttonEvents.size(), 1);
    EXPECT_EQ(motionEvents.size(), 1);
    EXPECT_EQ(wheelEvents.size(), 1);
}

TEST_F(InputManagerTest, MultipleCallbacksIntegration)
{
    ASSERT_TRUE(inputManager->initialize().has_value());

    // 设置多个回调函数来跟踪事件顺序和内容
    std::vector<std::string> eventLog;
    int keyEventCount         = 0;
    int mouseButtonEventCount = 0;
    int mouseMotionEventCount = 0;
    int mouseWheelEventCount  = 0;

    // clang-format off
    inputManager->setKeyEventCallback([&](const KeyboardEvent& event)
    {
        keyEventCount++;
        eventLog.push_back(std::format("Key: {} {}", static_cast<int>(event.keyCode), event.isPressed ? "Down" : "Up"));
    });

    inputManager->setMouseButtonCallback([&](const MouseButtonEvent& event)
    {
        mouseButtonEventCount++;
        eventLog.push_back(std::format("MouseButton: {} {} at ({},{})", static_cast<int>(event.button), event.isPressed ? "Down" : "Up", event.x, event.y));
    });

    inputManager->setMouseMotionCallback([&](const MouseMotionEvent& event)
    {
        mouseMotionEventCount++;
        eventLog.push_back(std::format("MouseMotion: ({},{}) delta({},{})", event.x, event.y, event.deltaX, event.deltaY));
    });

    inputManager->setMouseWheelCallback([&](const MouseWheelEvent& event)
    {
        mouseWheelEventCount++;
        eventLog.push_back(std::format("MouseWheel: delta={} at ({},{})", event.deltaY, event.mouseX, event.mouseY));
    });
    // clang-format on

    // 模拟复杂的用户交互序列
    // 1. 按下 Ctrl 键
    auto ctrlDown = createKeyboardEvent(PlatformEventType::KeyDown, KeyCode::LeftCtrl, KeyModifier::Ctrl);
    inputManager->processInputEvent(ctrlDown);

    // 2. 移动鼠标
    auto mouseMove1 = createMouseMoveEvent(100.0f, 200.0f, 50.0f, 100.0f);
    inputManager->processInputEvent(mouseMove1);

    // 3. 按下鼠标左键
    auto mouseDown = createMouseButtonEvent(PlatformEventType::MouseButtonDown, MouseButton::Left, 100.0f, 200.0f);
    inputManager->processInputEvent(mouseDown);

    // 4. 拖拽鼠标
    auto mouseMove2 = createMouseMoveEvent(150.0f, 250.0f, 50.0f, 50.0f);
    inputManager->processInputEvent(mouseMove2);

    // 5. 滚动鼠标滚轮
    auto mouseWheel = createMouseWheelEvent(1.5f, 150.0f, 250.0f);
    inputManager->processInputEvent(mouseWheel);

    // 6. 释放鼠标左键
    auto mouseUp = createMouseButtonEvent(PlatformEventType::MouseButtonUp, MouseButton::Left, 150.0f, 250.0f);
    inputManager->processInputEvent(mouseUp);

    // 7. 释放 Ctrl 键
    auto ctrlUp = createKeyboardEvent(PlatformEventType::KeyUp, KeyCode::LeftCtrl, KeyModifier::None);
    inputManager->processInputEvent(ctrlUp);

    // 验证所有事件都被正确捕获
    EXPECT_EQ(keyEventCount, 2);         // Ctrl 按下和释放
    EXPECT_EQ(mouseButtonEventCount, 2); // 鼠标左键按下和释放
    EXPECT_EQ(mouseMotionEventCount, 2); // 两次鼠标移动
    EXPECT_EQ(mouseWheelEventCount, 1);  // 一次滚轮事件

    // 验证事件日志的顺序和内容
    // clang-format off
    ASSERT_EQ(eventLog.size(), 7);
    EXPECT_TRUE(eventLog[0].find(std::format("Key: {} Down", static_cast<int>(KeyCode::LeftCtrl))) != std::string::npos);
    EXPECT_TRUE(eventLog[1].find("MouseMotion: (100") != std::string::npos);
    EXPECT_TRUE(eventLog[2].find(std::format("MouseButton: {} Down", static_cast<int>(MouseButton::Left))) != std::string::npos);
    EXPECT_TRUE(eventLog[3].find("MouseMotion: (150") != std::string::npos);
    EXPECT_TRUE(eventLog[4].find("MouseWheel: delta=1.5") != std::string::npos);
    EXPECT_TRUE(eventLog[5].find(std::format("MouseButton: {} Up", static_cast<int>(MouseButton::Left))) != std::string::npos);
    EXPECT_TRUE(eventLog[6].find(std::format("Key: {} Up", static_cast<int>(KeyCode::LeftCtrl))) != std::string::npos);
    // clang-format on

    // 验证 InputManager 的状态查询功能仍然正常工作
    EXPECT_FALSE(inputManager->isKeyPressed(KeyCode::LeftCtrl));
    EXPECT_FALSE(inputManager->isMouseButtonPressed(MouseButton::Left));
    EXPECT_EQ(inputManager->getMousePosition().x, 150);
    EXPECT_EQ(inputManager->getMousePosition().y, 250);
    EXPECT_FLOAT_EQ(inputManager->getMouseWheelDelta(), 1.5f);
}
