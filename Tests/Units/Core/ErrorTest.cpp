/**
 * @File ErrorTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#include <chrono>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <type_traits>

#include <Core/Error/Exception.hpp>

using namespace Bee;

TEST(ExceptionClassTest, ExceptionBasicFunctionality)
{
    Exception defaultException;
    EXPECT_STREQ(defaultException.what(), "");

    const std::string testMessage = "Test exception message";
    Exception messageException(testMessage);
    EXPECT_STREQ(messageException.what(), testMessage.c_str());

    Exception copiedException(messageException);
    EXPECT_STREQ(copiedException.what(), testMessage.c_str());

    static_assert(std::is_base_of_v<std::exception, Exception>);
}

TEST(ExceptionClassTest, RuntimeErrorFunctionality)
{
    RuntimeError defaultError;
    EXPECT_STREQ(defaultError.what(), "");

    const std::string testMessage = "Runtime error occurred";
    RuntimeError messageError(testMessage);
    EXPECT_STREQ(messageError.what(), testMessage.c_str());

    RuntimeError copiedError(messageError);
    EXPECT_STREQ(copiedError.what(), testMessage.c_str());

    static_assert(std::is_base_of_v<Exception, RuntimeError>);
    static_assert(std::is_base_of_v<std::exception, RuntimeError>);
}

TEST(ExceptionClassTest, AssertionErrorFunctionality)
{
    AssertionError defaultError;
    EXPECT_STREQ(defaultError.what(), "");

    const std::string testMessage = "Assertion failed";
    AssertionError messageError(testMessage);
    EXPECT_STREQ(messageError.what(), testMessage.c_str());

    AssertionError copiedError(messageError);
    EXPECT_STREQ(copiedError.what(), testMessage.c_str());

    static_assert(std::is_base_of_v<Exception, AssertionError>);
    static_assert(std::is_base_of_v<std::exception, AssertionError>);
}

TEST(ThrowMacroTest, BeeThrowBasicMessage)
{
    EXPECT_THROW({ BEE_THROW("Test error message"); }, RuntimeError);

    try
    {
        BEE_THROW("Test error message");
    }
    catch (const RuntimeError& e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find("Test error message") != std::string::npos);
        EXPECT_TRUE(errorMsg.find("ErrorTest.cpp") != std::string::npos);
    }
}

TEST(ThrowMacroTest, BeeThrowFormattedMessage)
{
    const int value        = 42;
    const std::string name = "test";

    EXPECT_THROW({ BEE_THROW("Value {} for {} is invalid", value, name); }, RuntimeError);

    try
    {
        BEE_THROW("Value {} for {} is invalid", value, name);
    }
    catch (const RuntimeError& e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find("Value 42 for test is invalid") != std::string::npos);
    }
}

TEST(ThrowMacroTest, BeeCheckTrueCondition)
{
    EXPECT_NO_THROW({
            BEE_CHECK(true, "This should not throw");
            BEE_CHECK(1 == 1, "This should not throw");
            BEE_CHECK(5 > 3, "This should not throw");
            });
}

// 测试 BEE_CHECK 宏 - 条件为假时抛出异常
TEST(ThrowMacroTest, BeeCheckFalseCondition)
{
    EXPECT_THROW({ BEE_CHECK(false, "Condition failed"); }, RuntimeError);

    EXPECT_THROW({ BEE_CHECK(1 == 2, "Numbers are not equal"); }, RuntimeError);

    try
    {
        BEE_CHECK(5 < 3, "Five is not less than three");
    }
    catch (const RuntimeError& e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find("Five is not less than three") != std::string::npos);
    }
}

TEST(ThrowMacroTest, BeeCheckFormattedMessage)
{
    const int expected = 10;
    const int actual   = 5;

    EXPECT_THROW({
                 BEE_CHECK(actual == expected, "Expected {}, but got {}", expected, actual);
                 }, RuntimeError);

    try
    {
        BEE_CHECK(actual == expected, "Expected {}, but got {}", expected, actual);
    }
    catch (const RuntimeError& e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find("Expected 10, but got 5") != std::string::npos);
    }
}

#ifdef BEE_DEBUG
TEST(DebugAssertMacroTest, DebugAssertInDebugMode)
{
    EXPECT_NO_THROW({
            BEE_DEBUG_ASSERT(true);
            BEE_DEBUG_ASSERT(1 == 1);
            });

    EXPECT_THROW({ BEE_DEBUG_ASSERT(false); }, AssertionError);
    EXPECT_THROW({ BEE_DEBUG_ASSERT(1 == 2, "Debug assertion failed"); }, AssertionError);
}

TEST(DebugAssertMacroTest, DebugAssertOpInDebugMode)
{
    EXPECT_NO_THROW({
            BEE_DEBUG_ASSERT_EQ(5, 5);
            BEE_DEBUG_ASSERT_NE(5, 10);
            BEE_DEBUG_ASSERT_LT(3, 5);
            BEE_DEBUG_ASSERT_LE(3, 5);
            BEE_DEBUG_ASSERT_GT(5, 3);
            BEE_DEBUG_ASSERT_GE(5, 3);
            });

    EXPECT_THROW({ BEE_DEBUG_ASSERT_EQ(5, 10); }, AssertionError);

    EXPECT_THROW({ BEE_DEBUG_ASSERT_NE(5, 5); }, AssertionError);
}

#endif

TEST(ErrorHandlerTest, CustomTerminateHandlerBehavior)
{
    auto originalHandler = std::get_terminate();

    std::set_terminate(Bee::details::CustomTerminateHandler);

    auto currentHandler = std::get_terminate();
    EXPECT_NE(currentHandler, originalHandler);
    
    std::set_terminate(originalHandler);
}

TEST(ErrorHandlerTest, SignalHandlerSetup)
{
    auto originalHandler = std::signal(SIGABRT, SIG_DFL);

    std::signal(SIGABRT, Bee::details::SignalHandler);

    auto currentHandler = std::signal(SIGABRT, SIG_DFL);
    EXPECT_EQ(currentHandler, Bee::details::SignalHandler);

    std::signal(SIGABRT, originalHandler);
}

TEST(SourceLocationTest, ExceptionContainsSourceLocation)
{
    try
    {
        BEE_THROW("Source location test");
        FAIL() << "Exception should have been thrown";
    }
    catch (const RuntimeError& e)
    {
        std::string errorMsg = e.what();

        EXPECT_TRUE(errorMsg.find("Source location test") != std::string::npos);
        EXPECT_TRUE(errorMsg.find("ErrorTest.cpp") != std::string::npos);
    }
}

TEST(SourceLocationTest, AssertionContainsSourceLocation)
{
    try
    {
        BEE_ASSERT(false, "Assertion source location test");
        FAIL() << "AssertionError should have been thrown";
    }
    catch (const AssertionError& e)
    {
        std::string errorMsg = e.what();

        EXPECT_TRUE(errorMsg.find("ErrorTest.cpp") != std::string::npos);
        EXPECT_TRUE(errorMsg.find("false") != std::string::npos);
        EXPECT_TRUE(errorMsg.find("Assertion source location test") != std::string::npos);
    }
}

TEST(ExceptionSafetyTest, ExceptionCopyAndMove)
{
    RuntimeError original("Original error message");
    
    RuntimeError copied(original);
    EXPECT_STREQ(original.what(), copied.what());
    
    RuntimeError moved(std::move(copied));
    EXPECT_STREQ(original.what(), moved.what());
}

TEST(ExceptionSafetyTest, RAIISafety)
{
    bool resourceReleased = false;

    // clang-format off
    class RAIIResource {
    public:
        RAIIResource(bool& flag) : released(flag) {}
        ~RAIIResource() { released = true; }
    private:
        bool& released;
    };
    // clang-format on
    
    try {
        RAIIResource resource(resourceReleased);
        BEE_THROW("Test RAII safety");
    } catch (const RuntimeError&) {
    }
    
    EXPECT_TRUE(resourceReleased);
}

TEST(MacroIntegrationTest, MacroWithGuardian)
{
    std::ostringstream capturedOutput;
    std::streambuf* originalCerr = std::cerr.rdbuf();
    std::cerr.rdbuf(capturedOutput.rdbuf());

    bool result = Guardian([]()
    {
        BEE_CHECK(false, "Check failed in Guardian");
        return true;
    });

    std::cerr.rdbuf(originalCerr);

    EXPECT_FALSE(result);
    std::string output = capturedOutput.str();
    EXPECT_TRUE(output.find("Exception:") != std::string::npos);
    EXPECT_TRUE(output.find("Check failed in Guardian") != std::string::npos);
}

TEST(MacroIntegrationTest, AssertMacroWithGuardian)
{
    std::ostringstream capturedOutput;
    std::streambuf* originalCerr = std::cerr.rdbuf();
    std::cerr.rdbuf(capturedOutput.rdbuf());

    bool result = Guardian([]()
    {
        BEE_ASSERT(1 == 2, "Assertion failed in Guardian");
        return true;
    });

    std::cerr.rdbuf(originalCerr);

    EXPECT_FALSE(result);
    std::string output = capturedOutput.str();
    EXPECT_TRUE(output.find("Assertion Failed:") != std::string::npos);
}

TEST(EdgeCaseTest, EmptyStringMessage)
{
    EXPECT_THROW({ BEE_THROW(""); }, RuntimeError);

    EXPECT_THROW({ BEE_CHECK(false, ""); }, RuntimeError);
}

TEST(EdgeCaseTest, LongMessage)
{
    std::string longMessage(1000, 'A');

    EXPECT_THROW({ BEE_THROW(longMessage); }, RuntimeError);

    try
    {
        BEE_THROW(longMessage);
    }
    catch (const RuntimeError& e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find(longMessage) != std::string::npos);
    }
}

TEST(EdgeCaseTest, SpecialCharacters)
{
    const std::string specialMsg = "Error with special chars: \n\t\r\"'\\";

    EXPECT_THROW({ BEE_THROW(specialMsg); }, RuntimeError);

    try
    {
        BEE_THROW(specialMsg);
    }
    catch (const RuntimeError& e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find(specialMsg) != std::string::npos);
    }
}
