/**
 * @File CheckTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/9
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include "Base/Check.hpp"

#include <cstdio>

// ============================================================================
// BEE_CHECK basic
// ============================================================================

TEST(CheckDeathTest, CheckFailsOnFalse)
{
    EXPECT_DEATH(BEE_CHECK(false), "Check failed: false");
}

TEST(CheckDeathTest, CheckPassesOnTrue)
{
    BEE_CHECK(true);
    BEE_CHECK(1 == 1);
    BEE_CHECK(42 > 0);
    SUCCEED();
}

TEST(CheckDeathTest, CheckMsgShowsMessage)
{
    EXPECT_DEATH(BEE_CHECK_MSG(1 > 2, "math is broken"), "math is broken");
}

TEST(CheckDeathTest, CheckMsgPassesOnTrue)
{
    BEE_CHECK_MSG(true, "should not fire");
    SUCCEED();
}

// ============================================================================
// BEE_CHECK_OP comparison macros
// ============================================================================

TEST(CheckDeathTest, CheckEqFailsWithValues)
{
    int x = 3, y = 5;
    EXPECT_DEATH(BEE_CHECK_EQ(x, y), "x == y");
}

TEST(CheckDeathTest, CheckEqPasses)
{
    BEE_CHECK_EQ(5, 5);
    BEE_CHECK_EQ(std::string("abc"), std::string("abc"));
    SUCCEED();
}

TEST(CheckDeathTest, CheckNeFails)
{
    EXPECT_DEATH(BEE_CHECK_NE(3, 3), "3 != 3");
}

TEST(CheckDeathTest, CheckLtFails)
{
    EXPECT_DEATH(BEE_CHECK_LT(5, 3), "5 < 3");
}

TEST(CheckDeathTest, CheckLeFails)
{
    EXPECT_DEATH(BEE_CHECK_LE(5, 3), "5 <= 3");
}

TEST(CheckDeathTest, CheckGtFails)
{
    EXPECT_DEATH(BEE_CHECK_GT(3, 5), "3 > 5");
}

TEST(CheckDeathTest, CheckGeFails)
{
    EXPECT_DEATH(BEE_CHECK_GE(3, 5), "3 >= 5");
}

TEST(CheckDeathTest, AllPassingOpChecks)
{
    BEE_CHECK_NE(1, 2);
    BEE_CHECK_LT(1, 2);
    BEE_CHECK_LE(2, 2);
    BEE_CHECK_GT(3, 2);
    BEE_CHECK_GE(2, 2);
    SUCCEED();
}

TEST(CheckDeathTest, OpMacroEvaluatesOnce)
{
    int counter = 0;
    auto inc    = [&]() {
        return ++counter;
    };
    BEE_CHECK_LE(inc(), 1);
    EXPECT_EQ(counter, 1);
}

// ============================================================================
// FailureHandler control API
// ============================================================================

TEST(CheckTest, DefaultHandlerIsNull)
{
    bee::SetFailureHandler(nullptr);
    EXPECT_EQ(bee::GetFailureHandler(), nullptr);
}

TEST(CheckTest, SetAndGetHandler)
{
    auto handler = [](std::string_view, std::string_view, std::source_location) noexcept {
    };
    bee::SetFailureHandler(handler);
    EXPECT_EQ(bee::GetFailureHandler(), handler);
    bee::SetFailureHandler(nullptr);
}

TEST(CheckDeathTest, FailureHandlerIsCalled)
{
    EXPECT_DEATH(
            {
            bee::SetFailureHandler([](std::string_view expr, std::string_view, std::source_location) noexcept {
                std::fprintf(stderr, "HANDLER_CALLED: %.*s\n",
                    static_cast<int>(expr.size()), expr.data());
                });
            BEE_CHECK(false);
            },
            "HANDLER_CALLED: false");
}

// ============================================================================
// BEE_ASSERT (debug-only)
// ============================================================================

#ifndef NDEBUG

TEST(CheckDeathTest, AssertFailsInDebug)
{
    EXPECT_DEATH(BEE_ASSERT(false), "Assertion failed: false");
}

TEST(CheckDeathTest, AssertMsgFailsInDebug)
{
    EXPECT_DEATH(BEE_ASSERT_MSG(false, "debug check"), "debug check");
}

TEST(CheckDeathTest, AssertEqFailsInDebug)
{
    int a = 1, b = 2;
    EXPECT_DEATH(BEE_ASSERT_EQ(a, b), "a == b");
}

#endif // NDEBUG

TEST(CheckTest, AssertPassesInDebug)
{
    BEE_ASSERT(true);
    BEE_ASSERT_MSG(true, "ok");
    BEE_ASSERT_EQ(1, 1);
    BEE_ASSERT_NE(1, 2);
    BEE_ASSERT_LT(1, 2);
    BEE_ASSERT_LE(1, 1);
    BEE_ASSERT_GT(2, 1);
    BEE_ASSERT_GE(2, 2);
    SUCCEED();
}

// ============================================================================
// BEE_UNREACHABLE
// ============================================================================

#ifndef NDEBUG

TEST(CheckDeathTest, UnreachableAborts)
{
    EXPECT_DEATH(BEE_UNREACHABLE(), "Unreachable");
}

#endif // NDEBUG
