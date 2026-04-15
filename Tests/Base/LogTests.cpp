/**
 * @File LogTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/9
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include "Base/Diagnostics/Log.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace
{

struct LogEntry
{
    bee::LogLevel        level;
    std::string          category;
    std::string          message;
    std::source_location location;
};

std::vector<LogEntry> g_captured;
std::mutex            g_captured_mutex;

void CaptureSink(bee::LogLevel level, std::string_view category, std::string_view message, std::source_location location)
{
    std::lock_guard lock(g_captured_mutex);
    g_captured.push_back({level, std::string(category), std::string(message), location});
}

class LogTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        std::lock_guard lock(g_captured_mutex);
        g_captured.clear();
        bee::SetLogSink(nullptr);
        bee::SetLogLevel(bee::LogLevel::Trace);
    }

    void TearDown() override
    {
        bee::SetLogSink(nullptr);
        bee::SetLogLevel(bee::LogLevel::Info);
    }
};

} // namespace

TEST_F(LogTest, DefaultSinkIsNull)
{
    bee::SetLogSink(nullptr);
    bee::LogRaw(bee::LogLevel::Info, "Test", "should not crash");
    EXPECT_TRUE(g_captured.empty());
}

TEST_F(LogTest, CustomSinkReceivesMessages)
{
    bee::SetLogSink(CaptureSink);
    bee::LogRaw(bee::LogLevel::Info, "Cache", "hello world");

    ASSERT_EQ(g_captured.size(), 1u);
    EXPECT_EQ(g_captured[0].level, bee::LogLevel::Info);
    EXPECT_EQ(g_captured[0].category, "Cache");
    EXPECT_EQ(g_captured[0].message, "hello world");
}

TEST_F(LogTest, LevelFiltering)
{
    bee::SetLogSink(CaptureSink);
    bee::SetLogLevel(bee::LogLevel::Warn);

    bee::LogRaw(bee::LogLevel::Trace, "A", "trace");
    bee::LogRaw(bee::LogLevel::Debug, "A", "debug");
    bee::LogRaw(bee::LogLevel::Info, "A", "info");
    bee::LogRaw(bee::LogLevel::Warn, "A", "warn");
    bee::LogRaw(bee::LogLevel::Error, "A", "error");
    bee::LogRaw(bee::LogLevel::Fatal, "A", "fatal");

    ASSERT_EQ(g_captured.size(), 3u);
    EXPECT_EQ(g_captured[0].level, bee::LogLevel::Warn);
    EXPECT_EQ(g_captured[1].level, bee::LogLevel::Error);
    EXPECT_EQ(g_captured[2].level, bee::LogLevel::Fatal);
}

TEST_F(LogTest, SetLogSinkNullDisables)
{
    bee::SetLogSink(CaptureSink);
    bee::LogRaw(bee::LogLevel::Info, "A", "before");
    ASSERT_EQ(g_captured.size(), 1u);

    bee::SetLogSink(nullptr);
    bee::LogRaw(bee::LogLevel::Info, "A", "after");
    EXPECT_EQ(g_captured.size(), 1u);
}

TEST_F(LogTest, GetLogSinkReturnsCurrentSink)
{
    EXPECT_EQ(bee::GetLogSink(), nullptr);
    bee::SetLogSink(CaptureSink);
    EXPECT_EQ(bee::GetLogSink(), CaptureSink);
}

TEST_F(LogTest, GetLogLevelReturnsCurrentLevel)
{
    bee::SetLogLevel(bee::LogLevel::Error);
    EXPECT_EQ(bee::GetLogLevel(), bee::LogLevel::Error);
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
    bee::SetLogSink(CaptureSink);
    bee::LogRaw(bee::LogLevel::Info, "Loc", "test");

    ASSERT_EQ(g_captured.size(), 1u);
    EXPECT_NE(std::string_view(g_captured[0].location.file_name()).find("LogTests"), std::string_view::npos);
    EXPECT_GT(g_captured[0].location.line(), 0u);
}

TEST_F(LogTest, LogFormatWithArgs)
{
    bee::SetLogSink(CaptureSink);
    bee::Log(bee::LogLevel::Info, "Math", "result: {}", 42);

    ASSERT_EQ(g_captured.size(), 1u);
    EXPECT_EQ(g_captured[0].message, "result: 42");
    EXPECT_EQ(g_captured[0].category, "Math");
}

TEST_F(LogTest, LogFormatMultipleArgs)
{
    bee::SetLogSink(CaptureSink);
    bee::Log(bee::LogLevel::Warn, "Cache", "ratio: {:.2f}, count: {}", 0.95, 100);

    ASSERT_EQ(g_captured.size(), 1u);
    EXPECT_EQ(g_captured[0].message, "ratio: 0.95, count: 100");
}

TEST_F(LogTest, LogFormatNoArgs)
{
    bee::SetLogSink(CaptureSink);
    bee::Log(bee::LogLevel::Info, "Test", "no args here");

    ASSERT_EQ(g_captured.size(), 1u);
    EXPECT_EQ(g_captured[0].message, "no args here");
}

TEST_F(LogTest, ConvenienceLogInfo)
{
    bee::SetLogSink(CaptureSink);
    bee::LogInfo("Module", "value: {}", 7);

    ASSERT_EQ(g_captured.size(), 1u);
    EXPECT_EQ(g_captured[0].level, bee::LogLevel::Info);
    EXPECT_EQ(g_captured[0].message, "value: 7");
}

TEST_F(LogTest, ConvenienceLogError)
{
    bee::SetLogSink(CaptureSink);
    bee::LogError("Net", "connection lost");

    ASSERT_EQ(g_captured.size(), 1u);
    EXPECT_EQ(g_captured[0].level, bee::LogLevel::Error);
    EXPECT_EQ(g_captured[0].message, "connection lost");
}

TEST_F(LogTest, EnableDefaultLoggingSetsLevelAndSink)
{
    bee::EnableDefaultLogging(bee::LogLevel::Warn);
    EXPECT_EQ(bee::GetLogSink(), bee::DefaultConsoleSink);
    EXPECT_EQ(bee::GetLogLevel(), bee::LogLevel::Warn);
}

TEST_F(LogTest, DefaultConsoleSinkDoesNotCrash)
{
    bee::EnableDefaultLogging(bee::LogLevel::Trace);
    bee::LogRaw(bee::LogLevel::Info, "Test", "console output test");
    SUCCEED();
}

TEST_F(LogTest, FormatLevelFilteringStillWorks)
{
    bee::SetLogSink(CaptureSink);
    bee::SetLogLevel(bee::LogLevel::Error);

    bee::Log(bee::LogLevel::Info, "A", "filtered: {}", 1);
    bee::Log(bee::LogLevel::Error, "A", "passed: {}", 2);

    ASSERT_EQ(g_captured.size(), 1u);
    EXPECT_EQ(g_captured[0].message, "passed: 2");
}
