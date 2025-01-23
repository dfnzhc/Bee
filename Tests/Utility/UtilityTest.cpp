/**
* @File LoggerTest.cpp
* @Author dfnzhc (https://github.com/dfnzhc)
* @Date 2024/11/14
* @Brief This file is part of Bee.
*/

#include <gtest/gtest.h>
#include <Utility/Logger.hpp>
#include <Utility/Enum.hpp>

using namespace bee;

TEST(LoggerTest, BasicOutput)
{
    int a         = 1;
    float b       = 2.f;
    std::string c = "3.0";

    LogDebug("Hello from info");
    LogDebug("Hello from info: {} {} {}", a, b, c);

    LogInfo("Hello from info");
    LogInfo("Hello from info: {} {} {}", a, b, c);

    LogWarn("Hello from warn");
    LogWarn("Hello from warn: {} {} {}", a, b, c);

    LogError("Hello from error");
    LogError("Hello from error: {} {} {}", a, b, c);

    LogError("Hello from fatal");
    LogFatal("Hello from fatal: {} {} {}", a, b, c);
}

namespace {
enum class Color : uint32_t
{
    Red    = 0x1,
    Green  = 0x2,
    Blue   = 0x4,
    Yellow = 0x8,
    
    RedGreen   = Red | Green,
    BlueYellow = Blue | Yellow,
    All        = Red | Green | Blue | Yellow
};
} // namespace

TEST(EnumTest, EnumToString)
{
    EXPECT_EQ(ToString(Color::Red), magic_enum::enum_name(Color::Red));
    EXPECT_EQ(ToString(Color::Green), magic_enum::enum_name(Color::Green));
    EXPECT_EQ(ToString(Color::Blue), magic_enum::enum_name(Color::Blue));
    EXPECT_EQ(ToString(Color::Yellow), magic_enum::enum_name(Color::Yellow));
    EXPECT_EQ(ToString(Color::RedGreen), magic_enum::enum_name(Color::RedGreen));
    EXPECT_EQ(ToString(Color::BlueYellow), magic_enum::enum_name(Color::BlueYellow));
    EXPECT_EQ(ToString(Color::All), magic_enum::enum_name(Color::All));
}

TEST(EnumTest, EnumInSet)
{
    EXPECT_TRUE(InSet(Color::Red, Color::Red));
    EXPECT_FALSE(InSet(Color::Red, Color::Green));
    EXPECT_FALSE(InSet(Color::Red, Color::Blue));
    EXPECT_FALSE(InSet(Color::Red, Color::Yellow));

    EXPECT_TRUE(InSet(Color::RedGreen, Color::Red));
    EXPECT_TRUE(InSet(Color::RedGreen, Color::Green));
    EXPECT_FALSE(InSet(Color::RedGreen, Color::Blue));
    EXPECT_FALSE(InSet(Color::RedGreen, Color::Yellow));

    EXPECT_FALSE(InSet(Color::BlueYellow, Color::Red));
    EXPECT_FALSE(InSet(Color::BlueYellow, Color::Green));
    EXPECT_TRUE(InSet(Color::BlueYellow, Color::Blue));
    EXPECT_TRUE(InSet(Color::BlueYellow, Color::Yellow));

    EXPECT_TRUE(InSet(Color::All, Color::Red));
    EXPECT_TRUE(InSet(Color::All, Color::Green));
    EXPECT_TRUE(InSet(Color::All, Color::Blue));
    EXPECT_TRUE(InSet(Color::All, Color::Yellow));
}