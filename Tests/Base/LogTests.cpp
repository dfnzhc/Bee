/**
 * @File LogTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/9
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include "Base/Diagnostics/Log.hpp"

#include <atomic>
#include <format>
#include <mutex>
#include <string>
#include <vector>

namespace
{

struct CountingArg
{
    int value;
};

std::atomic<int> gCountingArgFormatCalls{0};

struct LogEntry
{
    bee::LogLevel        level;
    std::string          category;
    std::string          message;
    std::source_location location;
};

std::vector<LogEntry> gCaptured;
std::mutex            gCapturedMutex;

void CaptureSink(bee::LogLevel level, std::string_view category, std::string_view message, std::source_location location)
{
    std::lock_guard lock(gCapturedMutex);
    gCaptured.push_back({level, std::string(category), std::string(message), location});
}

class LogTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        gCountingArgFormatCalls.store(0, std::memory_order_relaxed);
        std::lock_guard lock(gCapturedMutex);
        gCaptured.clear();
        bee::set_log_sink(nullptr);
        bee::set_log_level(bee::LogLevel::Trace);
    }

    void TearDown() override
    {
        bee::set_log_sink(nullptr);
        bee::set_log_level(bee::LogLevel::Info);
    }
};

} // namespace

template <>
struct std::formatter<CountingArg, char>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const CountingArg& arg, std::format_context& ctx) const
    {
        gCountingArgFormatCalls.fetch_add(1, std::memory_order_relaxed);
        return std::format_to(ctx.out(), "{}", arg.value);
    }
};

TEST_F(LogTest, DefaultSinkIsNull)
{
    bee::set_log_sink(nullptr);
    bee::LogRaw(bee::LogLevel::Info, "Test", "should not crash");
    EXPECT_TRUE(gCaptured.empty());
}

TEST_F(LogTest, CustomSinkReceivesMessages)
{
    bee::set_log_sink(CaptureSink);
    bee::LogRaw(bee::LogLevel::Info, "Cache", "hello world");

    ASSERT_EQ(gCaptured.size(), 1u);
    EXPECT_EQ(gCaptured[0].level, bee::LogLevel::Info);
    EXPECT_EQ(gCaptured[0].category, "Cache");
    EXPECT_EQ(gCaptured[0].message, "hello world");
}

TEST_F(LogTest, LevelFiltering)
{
    bee::set_log_sink(CaptureSink);
    bee::set_log_level(bee::LogLevel::Warn);

    bee::LogRaw(bee::LogLevel::Trace, "A", "trace");
    bee::LogRaw(bee::LogLevel::Debug, "A", "debug");
    bee::LogRaw(bee::LogLevel::Info, "A", "info");
    bee::LogRaw(bee::LogLevel::Warn, "A", "warn");
    bee::LogRaw(bee::LogLevel::Error, "A", "error");
    bee::LogRaw(bee::LogLevel::Fatal, "A", "fatal");

    ASSERT_EQ(gCaptured.size(), 3u);
    EXPECT_EQ(gCaptured[0].level, bee::LogLevel::Warn);
    EXPECT_EQ(gCaptured[1].level, bee::LogLevel::Error);
    EXPECT_EQ(gCaptured[2].level, bee::LogLevel::Fatal);
}

TEST_F(LogTest, SetLogSinkNullDisables)
{
    bee::set_log_sink(CaptureSink);
    bee::LogRaw(bee::LogLevel::Info, "A", "before");
    ASSERT_EQ(gCaptured.size(), 1u);

    bee::set_log_sink(nullptr);
    bee::LogRaw(bee::LogLevel::Info, "A", "after");
    EXPECT_EQ(gCaptured.size(), 1u);
}

TEST_F(LogTest, GetLogSinkReturnsCurrentSink)
{
    EXPECT_EQ(bee::get_log_sink(), nullptr);
    bee::set_log_sink(CaptureSink);
    EXPECT_EQ(bee::get_log_sink(), CaptureSink);
}

TEST_F(LogTest, GetLogLevelReturnsCurrentLevel)
{
    bee::set_log_level(bee::LogLevel::Error);
    EXPECT_EQ(bee::get_log_level(), bee::LogLevel::Error);
}

TEST_F(LogTest, LogLevelToStringAllLevels)
{
    EXPECT_EQ(bee::enum_to_name(bee::LogLevel::Trace), "Trace");
    EXPECT_EQ(bee::enum_to_name(bee::LogLevel::Debug), "Debug");
    EXPECT_EQ(bee::enum_to_name(bee::LogLevel::Info), "Info");
    EXPECT_EQ(bee::enum_to_name(bee::LogLevel::Warn), "Warn");
    EXPECT_EQ(bee::enum_to_name(bee::LogLevel::Error), "Error");
    EXPECT_EQ(bee::enum_to_name(bee::LogLevel::Fatal), "Fatal");
}

TEST_F(LogTest, LogRawPassesSourceLocation)
{
    bee::set_log_sink(CaptureSink);
    bee::LogRaw(bee::LogLevel::Info, "Loc", "test");

    ASSERT_EQ(gCaptured.size(), 1u);
    EXPECT_NE(std::string_view(gCaptured[0].location.file_name()).find("LogTests"), std::string_view::npos);
    EXPECT_GT(gCaptured[0].location.line(), 0u);
}

TEST_F(LogTest, LogFormatWithArgs)
{
    bee::set_log_sink(CaptureSink);
    bee::Log(bee::LogLevel::Info, "Math", "result: {}", 42);

    ASSERT_EQ(gCaptured.size(), 1u);
    EXPECT_EQ(gCaptured[0].message, "result: 42");
    EXPECT_EQ(gCaptured[0].category, "Math");
}

TEST_F(LogTest, LogFormatMultipleArgs)
{
    bee::set_log_sink(CaptureSink);
    bee::Log(bee::LogLevel::Warn, "Cache", "ratio: {:.2f}, count: {}", 0.95, 100);

    ASSERT_EQ(gCaptured.size(), 1u);
    EXPECT_EQ(gCaptured[0].message, "ratio: 0.95, count: 100");
}

TEST_F(LogTest, LogFormatNoArgs)
{
    bee::set_log_sink(CaptureSink);
    bee::Log(bee::LogLevel::Info, "Test", "no args here");

    ASSERT_EQ(gCaptured.size(), 1u);
    EXPECT_EQ(gCaptured[0].message, "no args here");
}

TEST_F(LogTest, ConvenienceLogInfo)
{
    bee::set_log_sink(CaptureSink);
    bee::LogInfo("Module", "value: {}", 7);

    ASSERT_EQ(gCaptured.size(), 1u);
    EXPECT_EQ(gCaptured[0].level, bee::LogLevel::Info);
    EXPECT_EQ(gCaptured[0].message, "value: 7");
}

TEST_F(LogTest, ConvenienceLogError)
{
    bee::set_log_sink(CaptureSink);
    bee::LogError("Net", "connection lost");

    ASSERT_EQ(gCaptured.size(), 1u);
    EXPECT_EQ(gCaptured[0].level, bee::LogLevel::Error);
    EXPECT_EQ(gCaptured[0].message, "connection lost");
}

TEST_F(LogTest, EnableDefaultLoggingSetsLevelAndSink)
{
    bee::enable_default_logging(bee::LogLevel::Warn);
    EXPECT_EQ(bee::get_log_sink(), bee::default_console_sink);
    EXPECT_EQ(bee::get_log_level(), bee::LogLevel::Warn);
}

TEST_F(LogTest, DefaultConsoleSinkDoesNotCrash)
{
    bee::enable_default_logging(bee::LogLevel::Trace);
    bee::LogRaw(bee::LogLevel::Info, "Test", "console output test");
    SUCCEED();
}

TEST_F(LogTest, FormatLevelFilteringStillWorks)
{
    bee::set_log_sink(CaptureSink);
    bee::set_log_level(bee::LogLevel::Error);

    bee::Log(bee::LogLevel::Info, "A", "filtered: {}", 1);
    bee::Log(bee::LogLevel::Error, "A", "passed: {}", 2);

    ASSERT_EQ(gCaptured.size(), 1u);
    EXPECT_EQ(gCaptured[0].message, "passed: 2");
}

TEST_F(LogTest, ConvenienceLogDoesNotFormatWhenFilteredOut)
{
    bee::set_log_sink(CaptureSink);
    bee::set_log_level(bee::LogLevel::Error);

    bee::LogInfo("A", "{}", CountingArg{7});

    EXPECT_TRUE(gCaptured.empty());
    EXPECT_EQ(gCountingArgFormatCalls.load(std::memory_order_relaxed), 0);
}

TEST_F(LogTest, ConvenienceLogFormatsWhenLevelPasses)
{
    bee::set_log_sink(CaptureSink);
    bee::set_log_level(bee::LogLevel::Trace);

    bee::LogInfo("A", "{}", CountingArg{9});

    ASSERT_EQ(gCaptured.size(), 1u);
    EXPECT_EQ(gCaptured[0].message, "9");
    EXPECT_EQ(gCountingArgFormatCalls.load(std::memory_order_relaxed), 1);
}
