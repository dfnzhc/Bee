/**
 * @File InputsTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#include <unordered_set>
#include <gtest/gtest.h>
#include <IO/Inputs.hpp>

using namespace bee;

TEST(MouseEventTest, EqualityOperator)
{
    MouseEvent input1;
    input1.type       = MouseEvent::Type::ButtonDown;
    input1.pos        = {0.5f, 0.5f};
    input1.screenPos  = {100.0f, 100.0f};
    input1.wheelDelta = {0.0f, 0.0f};
    input1.button     = MouseEvent::Button::Left;

    MouseEvent input2 = input1;

    EXPECT_TRUE(input1 == input2);
}

TEST(MouseEventTest, InequalityOperator)
{
    MouseEvent input1;
    input1.type       = MouseEvent::Type::ButtonDown;
    input1.pos        = {0.5f, 0.5f};
    input1.screenPos  = {100.0f, 100.0f};
    input1.wheelDelta = {0.0f, 0.0f};
    input1.button     = MouseEvent::Button::Left;

    MouseEvent input2;
    input2.type = MouseEvent::Type::ButtonUp;

    EXPECT_FALSE(input1 == input2);
}

TEST(MouseEventHashTest, DifferentEventsHaveDifferentHashes)
{
    bee::MouseEvent event1{
      bee::MouseEvent::Type::ButtonDown, {0.5, 0.5},
       {100, 100},
       {  0,   0}
    };
    bee::MouseEvent event2{
      bee::MouseEvent::Type::ButtonUp, {0.5, 0.5},
       {100, 100},
       {  0,   0}
    };

    std::hash<bee::MouseEvent> hasher;
    EXPECT_NE(hasher(event1), hasher(event2));
}

TEST(MouseEventHashTest, SameEventsHaveSameHashes)
{
    bee::MouseEvent event1{
      bee::MouseEvent::Type::ButtonDown, {0.5, 0.5},
       {100, 100},
       {  0,   0}
    };
    bee::MouseEvent event2{
      bee::MouseEvent::Type::ButtonDown, {0.5, 0.5},
       {100, 100},
       {  0,   0}
    };

    std::hash<bee::MouseEvent> hasher;
    EXPECT_EQ(hasher(event1), hasher(event2));
}

TEST(MouseInputTest, MouseButtonEvent)
{
    MouseInput mouseInput;
    MouseEvent mouseEvent;
    mouseEvent.type   = MouseEvent::Type::ButtonDown;
    mouseEvent.button = MouseEvent::Button::Left;

    mouseInput.onMouseEvent(mouseEvent);

    EXPECT_TRUE(mouseInput.isButtonDown(MouseEvent::Button::Left));
    EXPECT_FALSE(mouseInput.wasButtonDown(MouseEvent::Button::Left));

    EXPECT_TRUE(mouseInput.hasButtonDown());
    EXPECT_FALSE(mouseInput.hasButtonUp());

    EXPECT_FALSE(mouseInput.isButtonRepeated(MouseEvent::Button::Left));
    EXPECT_TRUE(mouseInput.isButtonClicked(MouseEvent::Button::Left));
    EXPECT_FALSE(mouseInput.isButtonReleased(MouseEvent::Button::Left));

    mouseInput.tick();
    mouseInput.onMouseEvent(mouseEvent);

    EXPECT_TRUE(mouseInput.isButtonDown(MouseEvent::Button::Left));
    EXPECT_TRUE(mouseInput.wasButtonDown(MouseEvent::Button::Left));

    EXPECT_TRUE(mouseInput.hasButtonDown());
    EXPECT_FALSE(mouseInput.hasButtonUp());

    EXPECT_TRUE(mouseInput.isButtonRepeated(MouseEvent::Button::Left));
    EXPECT_FALSE(mouseInput.isButtonClicked(MouseEvent::Button::Left));
    EXPECT_FALSE(mouseInput.isButtonReleased(MouseEvent::Button::Left));

    mouseInput.tick();
    mouseEvent.type = MouseEvent::Type::ButtonUp;
    mouseInput.onMouseEvent(mouseEvent);

    EXPECT_TRUE(mouseInput.isButtonUp(MouseEvent::Button::Left));
    EXPECT_TRUE(mouseInput.wasButtonDown(MouseEvent::Button::Left));

    EXPECT_FALSE(mouseInput.hasButtonDown());
    EXPECT_TRUE(mouseInput.hasButtonUp());

    EXPECT_FALSE(mouseInput.isButtonRepeated(MouseEvent::Button::Left));
    EXPECT_FALSE(mouseInput.isButtonClicked(MouseEvent::Button::Left));
    EXPECT_TRUE(mouseInput.isButtonReleased(MouseEvent::Button::Left));
}

TEST(MouseInputTest, MouseMoving)
{
    MouseInput mouseInput;
    MouseEvent mouseEvent;
    mouseEvent.type = MouseEvent::Type::Move;
    mouseInput.onMouseEvent(mouseEvent);

    EXPECT_TRUE(mouseInput.isMoving());
    mouseInput.tick();
    EXPECT_FALSE(mouseInput.isMoving());
}

TEST(MouseInputTest, MouseDragging)
{
    MouseInput mouseInput;
    MouseEvent mouseEvent;
    mouseEvent.type = MouseEvent::Type::Move;
    mouseInput.onMouseEvent(mouseEvent);

    mouseEvent.button = MouseEvent::Button::Left;
    mouseEvent.type   = MouseEvent::Type::ButtonDown;
    mouseInput.onMouseEvent(mouseEvent);

    EXPECT_TRUE(mouseInput.isDragging(MouseEvent::Button::Left));
    mouseInput.tick();
    EXPECT_FALSE(mouseInput.isDragging(MouseEvent::Button::Left));
}

TEST(MouseInputTest, MouseScrolled)
{
    MouseInput mouseInput;
    MouseEvent mouseEvent;
    mouseEvent.type = MouseEvent::Type::Wheel;
    mouseInput.onMouseEvent(mouseEvent);
    EXPECT_FALSE(mouseInput.isWheelScrolling());

    mouseEvent.button = MouseEvent::Button::Middle;
    mouseInput.onMouseEvent(mouseEvent);

    EXPECT_TRUE(mouseInput.isWheelScrolling());
    mouseInput.tick();
    EXPECT_FALSE(mouseInput.isWheelScrolling());
}

TEST(MouseInputTest, PosDelta)
{
    MouseInput mouseInput;
    MouseEvent mouseEvent1, mouseEvent2;

    mouseEvent1.type = MouseEvent::Type::Move;
    mouseEvent1.pos  = {0.1f, 0.1f};
    mouseInput.onMouseEvent(mouseEvent1);
    mouseInput.tick();

    mouseEvent2.type = MouseEvent::Type::Move;
    mouseEvent2.pos  = {0.2f, 0.3f};
    mouseInput.onMouseEvent(mouseEvent2);

    auto delta = mouseInput.posDelta();
    ASSERT_TRUE(delta);
    EXPECT_FLOAT_EQ(delta.value().x, 0.1f);
    EXPECT_FLOAT_EQ(delta.value().y, 0.2f);
}

TEST(MouseInputTest, ScreenPosDelta)
{
    MouseInput mouseInput;
    MouseEvent mouseEvent1, mouseEvent2;

    mouseEvent1.type      = MouseEvent::Type::Move;
    mouseEvent1.screenPos = {10.0f, 10.0f};
    mouseInput.onMouseEvent(mouseEvent1);
    mouseInput.tick();

    mouseEvent2.type      = MouseEvent::Type::Move;
    mouseEvent2.screenPos = {20.0f, 30.0f};
    mouseInput.onMouseEvent(mouseEvent2);

    auto delta = mouseInput.screenPosDelta();
    ASSERT_TRUE(delta);
    EXPECT_EQ(delta.value(), vec2(10.0f, 20.0f));
}

TEST(MouseInputTest, WheelDelta)
{
    MouseInput mouseInput;
    MouseEvent mouseEvent1, mouseEvent2;

    mouseEvent1.type       = MouseEvent::Type::Wheel;
    mouseEvent1.button     = MouseEvent::Button::Middle;
    mouseEvent1.wheelDelta = {1.0f, 0.0f};
    mouseInput.onMouseEvent(mouseEvent1);

    auto delta1 = mouseInput.wheelDelta();
    ASSERT_TRUE(delta1);
    EXPECT_EQ(delta1.value(), vec2(1.0f, 0.0f));

    mouseInput.tick();

    mouseEvent2.type       = MouseEvent::Type::Wheel;
    mouseEvent2.button     = MouseEvent::Button::Middle;
    mouseEvent2.wheelDelta = {2.0f, 1.0f};
    mouseInput.onMouseEvent(mouseEvent2);

    auto delta2 = mouseInput.wheelDelta();
    ASSERT_TRUE(delta2);
    EXPECT_EQ(delta2.value(), vec2(1.0f, 1.0f));
}

TEST(MouseInputTest, PosDeltaNoMovement)
{
    MouseInput mouseInput;
    MouseEvent mouseEvent;

    mouseEvent.pos  = {0.1f, 0.1f};
    mouseEvent.type = MouseEvent::Type::Move;
    mouseInput.onMouseEvent(mouseEvent);
    mouseInput.tick();

    auto delta = mouseInput.posDelta();
    EXPECT_TRUE(delta);
    EXPECT_EQ(delta.value(), vec2(0.0f, 0.0f));
}

TEST(MouseInputTest, ScreenPosDeltaNoMovement)
{
    MouseInput mouseInput;
    MouseEvent mouseEvent;

    mouseEvent.pos  = {10, 20};
    mouseEvent.type = MouseEvent::Type::Move;
    mouseInput.onMouseEvent(mouseEvent);
    mouseInput.tick();

    auto delta = mouseInput.screenPosDelta();
    EXPECT_TRUE(delta);
    EXPECT_EQ(delta.value(), vec2(0.0f, 0.0f));
}

TEST(KeyboardEventTest, EqualityOperator)
{
    KeyboardEvent event1;
    event1.type = KeyboardEvent::Type::Pressed;
    event1.key  = KeyboardEvent::Key::A;

    KeyboardEvent event2 = event1; // Copy constructor should make them equal
    EXPECT_TRUE(event1 == event2); // Both events should be equal
}

TEST(KeyboardEventHashTest, DifferentEventsHaveDifferentHashes)
{
    bee::KeyboardEvent event1{bee::KeyboardEvent::Type::Pressed, KeyboardEvent::Key::A};
    bee::KeyboardEvent event2{bee::KeyboardEvent::Type::Released, KeyboardEvent::Key::A};

    std::hash<bee::KeyboardEvent> hasher;
    EXPECT_NE(hasher(event1), hasher(event2));
}

TEST(KeyboardEventHashTest, SameEventsHaveSameHashes)
{
    bee::KeyboardEvent event1{bee::KeyboardEvent::Type::Pressed, bee::KeyboardEvent::Key::A};
    bee::KeyboardEvent event2{bee::KeyboardEvent::Type::Pressed, bee::KeyboardEvent::Key::A};

    std::hash<bee::KeyboardEvent> hasher;
    EXPECT_EQ(hasher(event1), hasher(event2));
}

TEST(KeyboardInputTest, KeyPressed)
{
    KeyboardInput keyboardInput;
    KeyboardEvent event;
    event.type = KeyboardEvent::Type::Pressed;
    event.key  = KeyboardEvent::Key::A;

    keyboardInput.onKeyboardEvent(event);
    keyboardInput.tick();

    EXPECT_TRUE(keyboardInput.isKeyPressed(KeyboardEvent::Key::A));
    EXPECT_FALSE(keyboardInput.isKeyReleased(KeyboardEvent::Key::A));
}

TEST(KeyboardInputTest, KeyReleased)
{
    KeyboardInput keyboardInput;
    KeyboardEvent event;
    event.type = KeyboardEvent::Type::Released;
    event.key  = KeyboardEvent::Key::B;

    keyboardInput.onKeyboardEvent(event);
    keyboardInput.tick();

    EXPECT_TRUE(keyboardInput.isKeyReleased(KeyboardEvent::Key::B));
    EXPECT_FALSE(keyboardInput.isKeyPressed(KeyboardEvent::Key::B));
}

TEST(KeyboardInputTest, KeyRepeated)
{
    KeyboardInput keyboardInput;
    KeyboardEvent event;
    event.type = KeyboardEvent::Type::Repeated;
    event.key  = KeyboardEvent::Key::C;

    keyboardInput.onKeyboardEvent(event);
    keyboardInput.tick();

    EXPECT_TRUE(keyboardInput.isKeyRepeated(KeyboardEvent::Key::C));
}

TEST(KeyboardInputTest, KeyInput)
{
    KeyboardInput keyboardInput;
    KeyboardEvent event;
    event.type = KeyboardEvent::Type::Input;
    event.key  = KeyboardEvent::Key::D;

    keyboardInput.onKeyboardEvent(event);
    keyboardInput.tick();

    EXPECT_TRUE(keyboardInput.isKeyInput(KeyboardEvent::Key::D));
}

TEST(KeyboardInputTest, Modifiers)
{
    KeyboardInput keyboardInput;
    keyboardInput.onKeyboardEvent({KeyboardEvent::Type::Pressed, KeyboardEvent::Key::LeftShift});
    keyboardInput.onKeyboardEvent({KeyboardEvent::Type::Pressed, KeyboardEvent::Key::LeftControl});
    keyboardInput.tick();

    EXPECT_TRUE(keyboardInput.hasModifier());
    EXPECT_TRUE(keyboardInput.hasModifier(KeyboardEvent::ModifierFlags::Shift));
    EXPECT_TRUE(keyboardInput.hasModifier(KeyboardEvent::ModifierFlags::Ctrl));
}

TEST(KeyboardInputTest, HasKeyPressedAfterTick)
{
    KeyboardInput keyboardInput;
    KeyboardEvent event;
    event.type = KeyboardEvent::Type::Pressed;
    event.key  = KeyboardEvent::Key::E;

    keyboardInput.onKeyboardEvent(event);
    keyboardInput.tick();

    EXPECT_TRUE(keyboardInput.wasKeyPressed(KeyboardEvent::Key::E));
}

TEST(KeyboardInputTest, NoKeyPressedAfterTick)
{
    KeyboardInput keyboardInput;
    KeyboardEvent event;
    event.type = KeyboardEvent::Type::Released;
    event.key  = KeyboardEvent::Key::F;

    keyboardInput.onKeyboardEvent(event);
    keyboardInput.tick();

    EXPECT_FALSE(keyboardInput.wasKeyPressed(KeyboardEvent::Key::F));
}

TEST(KeyboardInputTest, TestKeyTypeAnyKeyPressed)
{
    KeyboardInput keyboardInput;
    KeyboardEvent event;
    event.type = KeyboardEvent::Type::Pressed;
    event.key  = KeyboardEvent::Key::G;

    keyboardInput.onKeyboardEvent(event);
    keyboardInput.tick();

    EXPECT_TRUE(keyboardInput.hasKeyPressed());
}

TEST(KeyboardInputTest, TestKeyTypeAnyKeyReleased)
{
    KeyboardInput keyboardInput;
    KeyboardEvent event;
    event.type = KeyboardEvent::Type::Released;
    event.key  = KeyboardEvent::Key::H;

    keyboardInput.onKeyboardEvent(event);
    keyboardInput.tick();

    EXPECT_TRUE(keyboardInput.hasKeyReleased());
}

TEST(InputManagerTest, Subscribe)
{
    InputManager inputManager;

    MouseEvent me;
    me.button = MouseEvent::Button::Left;
    me.type   = MouseEvent::Type::ButtonDown;

    int cnt = 0;
    auto id = inputManager.subscribe([&](const MouseInput& mi, const KeyboardInput& ki) {
        if (mi.isButtonDown(MouseEvent::Button::Left))
            cnt += 1;
    });

    EXPECT_EQ(cnt, 0);
    for (int i = 0; i < 3; ++i) {
        inputManager.onMouseEvent(me);
        inputManager.tick();
    }
    EXPECT_EQ(cnt, 3);

    inputManager.unsubscribe(id);

    EXPECT_EQ(cnt, 3);
    for (int i = 0; i < 3; ++i) {
        inputManager.onMouseEvent(me);
        inputManager.tick();
    }
    EXPECT_EQ(cnt, 3);
}