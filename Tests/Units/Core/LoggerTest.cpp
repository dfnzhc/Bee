/**
 * @File LogTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/14
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <Core/Logger/Logger.hpp>

using namespace Bee;

class TestSink : public ILogSink
{
public:
    void write(const LogMessage& message) override
    {
        messages.push_back(message);
    }

    void flush() override
    {
    }

    bool isValid() const override
    {
        return true;
    }

    std::vector<LogMessage> messages;
};

class LoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Logger::Instance().clearSinks();
        testSink    = std::make_unique<TestSink>();
        testSinkPtr = testSink.get();
        Logger::Instance().addSink(std::move(testSink));
    }

    void TearDown() override
    {
        Logger::Instance().clearSinks();
    }

    std::unique_ptr<TestSink> testSink;
    TestSink* testSinkPtr{nullptr};
};

TEST_F(LoggerTest, BasicLogging)
{
    BEE_INFO("Test message");

    ASSERT_EQ(testSinkPtr->messages.size(), 1);
    EXPECT_EQ(testSinkPtr->messages[0].message, "Test message");
    EXPECT_EQ(testSinkPtr->messages[0].level, LogMessage::Level::Info);
}

TEST_F(LoggerTest, FormattedLogging) {
    BEE_INFO("Test message with value: {}", 42);
    
    ASSERT_EQ(testSinkPtr->messages.size(), 1);
    EXPECT_EQ(testSinkPtr->messages[0].message, "Test message with value: 42");
}

TEST_F(LoggerTest, LevelFiltering) {
    Logger::Instance().setMinLevel(LogMessage::Level::Warn);
    
    BEE_TRACE("Should be filtered");
    BEE_INFO("Should be filtered");
    BEE_WARN("Should pass");
    BEE_ERROR("Should pass");
    
    ASSERT_EQ(testSinkPtr->messages.size(), 2);
    EXPECT_EQ(testSinkPtr->messages[0].level, LogMessage::Level::Warn);
    EXPECT_EQ(testSinkPtr->messages[1].level, LogMessage::Level::Error);
}