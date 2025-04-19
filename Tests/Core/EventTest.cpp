/**
 * @File ErrorTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/31
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Events/EventManager.hpp>
#include <Events/InputEvents.hpp>

using namespace bee;

class DummyEvent : public Event
{
public:
    DummyEvent()
        : Event(EventType::keyboard)
    {
    }

    ~DummyEvent() override
    {
    }
};

TEST(EventSystemTest, AppendEvent)
{
    int k       = 0;
    auto handle = EventManager::Instance().Register(EventType::keyboard,
                                                   [&](const EventPtr& e) -> void {
                                                       k = 42;
                                                   });

    auto event = std::make_shared<DummyEvent>();
    EXPECT_EQ(k, 0);
    EventManager::Instance().Push(event);
    EventManager::Instance().Process();
    EXPECT_EQ(k, 42);

    k = 0;
    EventManager::Instance().Process();
    EXPECT_EQ(k, 0);

    EventManager::Instance().Push(event);
    EventManager::Instance().Process();
    EXPECT_EQ(k, 42);

    k = 0;
    EventManager::Instance().Unregister(handle);
    EventManager::Instance().Push(event);
    EventManager::Instance().Process();
    EXPECT_EQ(k, 0);
}


/// ==========================
/// Input Events
/// ==========================

TEST(ModifierKeysTest, CombinationCheck)
{
    BEE_USE_MAGIC_ENUM_BIT_OPERATOR;
    ModifierKeysState state(true, false, true, false, false, false);
    EXPECT_TRUE(state.AreModifersDown(ModifierKey::Shift | ModifierKey::Control));
    EXPECT_FALSE(state.AreModifersDown(ModifierKey::Alt));
}

TEST(ModifierKeysTest, ToStringFormat)
{
    ModifierKeysState state(true, false, true, false, false, true);
    EXPECT_EQ(state.toString(), "Modifiers: +LShift +LCtrl +RAlt");
}

TEST(MouseEventTest, ButtonPressDetection)
{
    MouseEvent event(100,
                     200,
                     MouseButton::Left,
                     InputType::MouseButtonDown,
                     1,
                     ModifierKeysState());
    EXPECT_EQ(event.button().value(), MouseButton::Left);
    EXPECT_EQ(event.clicks().value(), 1);
}

TEST(MouseEventTest, WheelDeltaValidation)
{
    MouseEvent event(0.5f, -1.2f, ModifierKeysState());
    const auto delta = event.wheelDelta().value();
    EXPECT_FLOAT_EQ(delta.x, 0.5f);
    EXPECT_FLOAT_EQ(delta.y, -1.2f);
}

TEST(MouseEventTest, MotionEventSafety)
{
    MouseEvent event1(0, 0, 4, 2, ModifierKeysState());
    EXPECT_TRUE(event1.relativeMotion().has_value());
    EXPECT_TRUE(event1.relativeMotion().value().x == 4);
    EXPECT_TRUE(event1.relativeMotion().value().y == 2);
    
    MouseEvent event2(0.5f, -1.2f, ModifierKeysState());
    EXPECT_FALSE(event2.relativeMotion().has_value());
}

TEST(MouseEventTest, MaxClicksBoundary)
{
    MouseEvent event(0,
                     0,
                     MouseButton::Left,
                     InputType::MouseButtonDown,
                     255,
                     ModifierKeysState());
    EXPECT_EQ(event.clicks().value(), 255);
}

TEST(ModifierKeysTest, InvalidModifierCombination)
{
    ModifierKeysState state;
    EXPECT_TRUE(state.AreModifersDown(ModifierKey::None));
}