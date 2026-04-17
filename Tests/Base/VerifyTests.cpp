/**
 * @File VerifyTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/6/22
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include "Base/Diagnostics/Check.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace
{

struct LogEntry
{
    bee::LogLevel level;
    std::string   category;
    std::string   message;
};

std::vector<LogEntry> gVerifyLogs;
std::mutex            gVerifyLogsMutex;

void VerifyCaptureSink(bee::LogLevel level, std::string_view category, std::string_view message, std::source_location)
{
    std::lock_guard lock(gVerifyLogsMutex);
    gVerifyLogs.push_back({level, std::string(category), std::string(message)});
}

class VerifyTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        std::lock_guard lock(gVerifyLogsMutex);
        gVerifyLogs.clear();
        bee::set_log_sink(VerifyCaptureSink);
        bee::set_log_level(bee::LogLevel::Trace);
    }

    void TearDown() override
    {
        bee::set_log_sink(nullptr);
        bee::set_log_level(bee::LogLevel::Info);
    }
};

} // namespace

// ============================================================================
// BEE_VERIFY (does NOT abort)
// ============================================================================

TEST_F(VerifyTest, VerifyPassesOnTrue)
{
    BEE_VERIFY(true);
    BEE_VERIFY(1 == 1);
    EXPECT_TRUE(gVerifyLogs.empty());
}

TEST_F(VerifyTest, VerifyFailsWithoutAborting)
{
    BEE_VERIFY(false);
    // Execution continues — no abort
    ASSERT_EQ(gVerifyLogs.size(), 1u);
    EXPECT_EQ(gVerifyLogs[0].level, bee::LogLevel::Warn);
    EXPECT_EQ(gVerifyLogs[0].category, "Verify");
    EXPECT_NE(gVerifyLogs[0].message.find("Verify failed: false"), std::string::npos);
}

TEST_F(VerifyTest, VerifyMsgIncludesMessage)
{
    BEE_VERIFY_MSG(1 > 2, "math sanity");
    ASSERT_EQ(gVerifyLogs.size(), 1u);
    EXPECT_NE(gVerifyLogs[0].message.find("math sanity"), std::string::npos);
}

TEST_F(VerifyTest, VerifyMsgPassesOnTrue)
{
    BEE_VERIFY_MSG(true, "should not appear");
    EXPECT_TRUE(gVerifyLogs.empty());
}

// ============================================================================
// BEE_VERIFY_OP comparison macros
// ============================================================================

TEST_F(VerifyTest, VerifyEqPassesOnEqual)
{
    BEE_VERIFY_EQ(5, 5);
    EXPECT_TRUE(gVerifyLogs.empty());
}

TEST_F(VerifyTest, VerifyEqFailsWithValues)
{
    int x = 3, y = 5;
    BEE_VERIFY_EQ(x, y);
    ASSERT_EQ(gVerifyLogs.size(), 1u);
    EXPECT_NE(gVerifyLogs[0].message.find("x == y"), std::string::npos);
    EXPECT_NE(gVerifyLogs[0].message.find("x = 3"), std::string::npos);
    EXPECT_NE(gVerifyLogs[0].message.find("y = 5"), std::string::npos);
}

TEST_F(VerifyTest, VerifyNeFailsOnEqual)
{
    BEE_VERIFY_NE(7, 7);
    ASSERT_EQ(gVerifyLogs.size(), 1u);
    EXPECT_NE(gVerifyLogs[0].message.find("!="), std::string::npos);
}

TEST_F(VerifyTest, VerifyLtFailsWhenGreater)
{
    BEE_VERIFY_LT(10, 5);
    ASSERT_EQ(gVerifyLogs.size(), 1u);
    EXPECT_NE(gVerifyLogs[0].message.find("<"), std::string::npos);
}

TEST_F(VerifyTest, VerifyLePassesOnEqual)
{
    BEE_VERIFY_LE(5, 5);
    EXPECT_TRUE(gVerifyLogs.empty());
}

TEST_F(VerifyTest, VerifyGtFailsWhenEqual)
{
    BEE_VERIFY_GT(3, 3);
    ASSERT_EQ(gVerifyLogs.size(), 1u);
}

TEST_F(VerifyTest, VerifyGePassesOnGreater)
{
    BEE_VERIFY_GE(10, 5);
    EXPECT_TRUE(gVerifyLogs.empty());
}

TEST_F(VerifyTest, VerifyGeFailsWhenLess)
{
    BEE_VERIFY_GE(3, 5);
    ASSERT_EQ(gVerifyLogs.size(), 1u);
}

// ============================================================================
// Multiple verifications in sequence (execution continues)
// ============================================================================

TEST_F(VerifyTest, MultipleFailuresContinueExecution)
{
    BEE_VERIFY(false);
    BEE_VERIFY_EQ(1, 2);
    BEE_VERIFY_MSG(false, "third failure");

    ASSERT_EQ(gVerifyLogs.size(), 3u);
    EXPECT_EQ(gVerifyLogs[0].level, bee::LogLevel::Warn);
    EXPECT_EQ(gVerifyLogs[1].level, bee::LogLevel::Warn);
    EXPECT_EQ(gVerifyLogs[2].level, bee::LogLevel::Warn);
}

// ============================================================================
// Side-effect safety
// ============================================================================

TEST_F(VerifyTest, VerifyOpEvaluatesOnce)
{
    int  counter = 0;
    auto inc     = [&]() {
        return ++counter;
    };
    BEE_VERIFY_LE(inc(), 1);
    EXPECT_EQ(counter, 1);
}
