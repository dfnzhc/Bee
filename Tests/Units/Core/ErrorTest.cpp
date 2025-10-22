/**
 * @File ErrorTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#include <atomic>
#include <chrono>
#include <numeric>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <Core/Error/Exception.hpp>
#include <Core/Error/Guardian.hpp>
#include <Core/Error/Error.hpp>

using namespace Bee;

// ==================== 异常、断言 ====================

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

    try
    {
        RAIIResource resource(resourceReleased);
        BEE_THROW("Test RAII safety");
    }
    catch (const RuntimeError&)
    {
    }

    EXPECT_TRUE(resourceReleased);
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

// ==================== 异常护卫 ====================

TEST(ExceptionGuardianTest, BasicFunctionality)
{
    auto result = ExceptionGuardian([]()
    {
        return 42;
    });

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
}

TEST(ExceptionGuardianTest, VoidFunctionExecution)
{
    bool executed = false;
    auto result   = ExceptionGuardian([&executed]()
    {
        executed = true;
    });

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(executed);
}

TEST(ExceptionGuardianTest, FunctionWithParameters)
{
    auto result = ExceptionGuardian([](int a, int b)
    {
        return a + b;
    }, 10, 20);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 30);
}

TEST(ExceptionGuardianTest, RuntimeErrorCatching)
{
    auto result = ExceptionGuardian([]()
    {
        BEE_THROW("Test runtime error");
        return 42;
    });

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error() != nullptr);

    try
    {
        std::rethrow_exception(result.error());
    }
    catch (const RuntimeError& e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find("Test runtime error") != std::string::npos);
    }
    catch (...)
    {
        FAIL() << "Expected RuntimeError but caught different exception type";
    }
}

TEST(ExceptionGuardianTest, AssertionErrorCatching)
{
    auto result = ExceptionGuardian([]()
    {
        BEE_ASSERT(false, "Test assertion failure");
        return "Never reached";
    });

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error() != nullptr);

    try
    {
        std::rethrow_exception(result.error());
    }
    catch (const AssertionError& e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find("false") != std::string::npos);
        EXPECT_TRUE(errorMsg.find("Test assertion failure") != std::string::npos);
    }
    catch (...)
    {
        FAIL() << "Expected AssertionError but caught different exception type";
    }
}

TEST(ExceptionGuardianTest, StandardExceptionCatching)
{
    auto result = ExceptionGuardian([]()
    {
        throw std::runtime_error("Standard runtime error");
        return 42;
    });

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error() != nullptr);

    try
    {
        std::rethrow_exception(result.error());
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Standard runtime error");
    }
    catch (...)
    {
        FAIL() << "Expected std::runtime_error but caught different exception type";
    }
}

TEST(ExceptionGuardianTest, UnknownExceptionCatching)
{
    auto result = ExceptionGuardian([]()
    {
        throw 42;
        return "Never reached";
    });

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error() != nullptr);

    try
    {
        std::rethrow_exception(result.error());
        FAIL() << "Expected exception to be rethrown";
    }
    catch (int value)
    {
        EXPECT_EQ(value, 42);
    }
    catch (...)
    {
        FAIL() << "Expected int exception but caught different type";
    }
}

TEST(ExceptionGuardianTest, VoidFunctionWithException)
{
    bool executed = false;
    auto result   = ExceptionGuardian([&executed]()
    {
        executed = true;
        BEE_THROW("Void function error");
    });

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error() != nullptr);
    EXPECT_TRUE(executed);
}

TEST(ExceptionGuardianTest, ComplexReturnTypes)
{
    auto result = ExceptionGuardian([]()
    {
        return std::make_pair(42, std::string("hello"));
    });

    EXPECT_TRUE(result.has_value());
    auto [num, str] = result.value();
    EXPECT_EQ(num, 42);
    EXPECT_EQ(str, "hello");
}

TEST(ExceptionGuardianTest, LambdaWithCapture)
{
    int multiplier = 5;
    auto result    = ExceptionGuardian([multiplier](int value)
    {
        return value * multiplier;
    }, 8);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 40);
}

TEST(ExceptionGuardianTest, FunctionPointer)
{
    auto add = [](int a, int b) -> int
    {
        return a + b;
    };
    auto result = ExceptionGuardian(add, 15, 25);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 40);
}

TEST(ExceptionGuardianTest, MemberFunctionCall)
{
    // clang-format off
    class TestClass
    {
    public:
        int getValue() const { return _value; }
        void setValue(int v) { _value = v; }
        void throwError() { BEE_THROW("Member function error"); }

    private:
        int _value = 100;
    };
    // clang-format on

    TestClass obj;

    auto result1 = ExceptionGuardian([&obj]()
    {
        return obj.getValue();
    });
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), 100);

    auto result2 = ExceptionGuardian([&obj]()
    {
        obj.throwError();
    });
    EXPECT_FALSE(result2.has_value());
}

TEST(ExceptionGuardianTest, NestedExceptionGuardian)
{
    auto result = ExceptionGuardian([]()
    {
        auto innerResult = ExceptionGuardian([]()
        {
            return 42;
        });

        if (innerResult.has_value())
        {
            return innerResult.value() * 2;
        }
        else
        {
            BEE_THROW("Inner execution failed");
            return 0; // Never reached
        }
    });

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 84);
}

TEST(ExceptionGuardianTest, ExceptionInNestedCall)
{
    auto result = ExceptionGuardian([]()
    {
        auto innerResult = ExceptionGuardian([]()
        {
            BEE_THROW("Inner error");
            return 42;
        });

        if (!innerResult.has_value())
        {
            BEE_THROW("Outer error due to inner failure");
        }

        return innerResult.value();
    });

    EXPECT_FALSE(result.has_value());

    try
    {
        std::rethrow_exception(result.error());
    }
    catch (const RuntimeError& e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find("Outer error due to inner failure") != std::string::npos);
    }
}

TEST(ExceptionGuardianTest, TypeDeduction)
{
    auto intResult = ExceptionGuardian([]()
    {
        return 42;
    });
    static_assert(std::is_same_v<decltype(intResult), std::expected<int, std::exception_ptr>>);

    auto stringResult = ExceptionGuardian([]()
    {
        return std::string("hello");
    });
    static_assert(std::is_same_v<decltype(stringResult), std::expected<std::string, std::exception_ptr>>);

    auto voidResult = ExceptionGuardian([]()
    {
        /* void function */
    });
    static_assert(std::is_same_v<decltype(voidResult), std::expected<void, std::exception_ptr>>);
}

// ==================== 作用域护卫 ====================

TEST(ScopeGuardianTest, BasicCleanupExecution)
{
    bool cleanupExecuted = false;

    {
        ScopeGuardian guard;
        guard.onCleanup([&cleanupExecuted]()
        {
            cleanupExecuted = true;
        });
    }

    EXPECT_TRUE(cleanupExecuted);
}

TEST(ScopeGuardianTest, MultipleCleanupCallbacks)
{
    std::vector<int> executionOrder;

    {
        ScopeGuardian guard;
        guard.onCleanup([&executionOrder]()
        {
            executionOrder.push_back(1);
        }, ScopeGuardian::CleanupTiming::Always, "first");

        guard.onCleanup([&executionOrder]()
        {
            executionOrder.push_back(2);
        }, ScopeGuardian::CleanupTiming::Always, "second");

        guard.onCleanup([&executionOrder]()
        {
            executionOrder.push_back(3);
        }, ScopeGuardian::CleanupTiming::Always, "third");
    }

    // 逆序
    ASSERT_EQ(executionOrder.size(), 3);
    EXPECT_EQ(executionOrder[0], 3);
    EXPECT_EQ(executionOrder[1], 2);
    EXPECT_EQ(executionOrder[2], 1);
}

TEST(ScopeGuardianTest, CleanupTimingAlways)
{
    bool cleanupExecuted = false;

    {
        ScopeGuardian guard;
        guard.onCleanup([&cleanupExecuted]()
        {
            cleanupExecuted = true;
        }, ScopeGuardian::CleanupTiming::Always);
    }

    EXPECT_TRUE(cleanupExecuted);
}

TEST(ScopeGuardianTest, CleanupTimingOnSuccess)
{
    bool successCleanupExecuted = false;
    bool failureCleanupExecuted = false;

    {
        ScopeGuardian guard;
        guard.onCleanup([&successCleanupExecuted]()
        {
            successCleanupExecuted = true;
        }, ScopeGuardian::CleanupTiming::OnSuccess);

        guard.onCleanup([&failureCleanupExecuted]()
        {
            failureCleanupExecuted = true;
        }, ScopeGuardian::CleanupTiming::OnFailure);
    }

    EXPECT_TRUE(successCleanupExecuted);
    EXPECT_FALSE(failureCleanupExecuted);
}

TEST(ScopeGuardianTest, CleanupTimingOnFailure)
{
    bool successCleanupExecuted = false;
    bool failureCleanupExecuted = false;

    try
    {
        ScopeGuardian guard;
        guard.onCleanup([&successCleanupExecuted]()
        {
            successCleanupExecuted = true;
        }, ScopeGuardian::CleanupTiming::OnSuccess);

        guard.onCleanup([&failureCleanupExecuted]()
        {
            failureCleanupExecuted = true;
        }, ScopeGuardian::CleanupTiming::OnFailure);

        throw std::runtime_error("Test exception");
    }
    catch (...)
    {
    }

    EXPECT_FALSE(successCleanupExecuted);
    EXPECT_TRUE(failureCleanupExecuted);
}

TEST(ScopeGuardianTest, CleanupTimingOnException)
{
    bool exceptionCleanupExecuted = false;
    bool normalCleanupExecuted    = false;

    try
    {
        ScopeGuardian guard;
        guard.onCleanup([&exceptionCleanupExecuted]()
        {
            exceptionCleanupExecuted = true;
        }, ScopeGuardian::CleanupTiming::OnException);

        guard.onCleanup([&normalCleanupExecuted]()
        {
            normalCleanupExecuted = true;
        }, ScopeGuardian::CleanupTiming::OnSuccess);

        throw std::runtime_error("Test exception");
    }
    catch (...)
    {
    }

    EXPECT_TRUE(exceptionCleanupExecuted);
    EXPECT_FALSE(normalCleanupExecuted);
}

TEST(ScopeGuardianTest, ExceptionCallback)
{
    bool exceptionCallbackExecuted = false;
    std::string caughtMessage;

    try
    {
        ScopeGuardian guard;
        guard.onException([&exceptionCallbackExecuted, &caughtMessage](const std::exception_ptr& ex)
        {
            exceptionCallbackExecuted = true;
            try
            {
                std::rethrow_exception(ex);
            }
            catch (const std::exception& e)
            {
                caughtMessage = e.what();
            }
        });

        guard.setException(std::make_exception_ptr(std::runtime_error("Test exception message")));
        // throw std::runtime_error("Test exception message");
    }
    catch (...)
    {
    }

    EXPECT_TRUE(exceptionCallbackExecuted);
    EXPECT_EQ(caughtMessage, "Test exception message");
}

TEST(ScopeGuardianTest, OnSuccessConvenienceMethod)
{
    bool successExecuted = false;

    {
        ScopeGuardian guard;
        guard.onSuccess([&successExecuted]()
        {
            successExecuted = true;
        });
    }

    EXPECT_TRUE(successExecuted);
}

TEST(ScopeGuardianTest, OnFailureConvenienceMethod)
{
    bool failureExecuted = false;

    try
    {
        ScopeGuardian guard;
        guard.onFailure([&failureExecuted]()
        {
            failureExecuted = true;
        });

        throw std::runtime_error("Test exception");
    }
    catch (...)
    {
        // 捕获异常以继续测试
    }

    EXPECT_TRUE(failureExecuted);
}

TEST(ScopeGuardianTest, ManualExceptionSetting)
{
    bool exceptionCallbackExecuted = false;
    bool failureCleanupExecuted    = false;
    bool successCleanupExecuted    = false;

    {
        ScopeGuardian guard;

        guard.onException([&exceptionCallbackExecuted](const std::exception_ptr&)
        {
            exceptionCallbackExecuted = true;
        });

        guard.onFailure([&failureCleanupExecuted]()
        {
            failureCleanupExecuted = true;
        });

        guard.onSuccess([&successCleanupExecuted]()
        {
            successCleanupExecuted = true;
        });

        guard.setException(std::make_exception_ptr(std::runtime_error("Manual exception")));
    }

    EXPECT_TRUE(exceptionCallbackExecuted);
    EXPECT_TRUE(failureCleanupExecuted);
    EXPECT_FALSE(successCleanupExecuted);
}

TEST(ScopeGuardianTest, DismissGuard)
{
    bool cleanupExecuted = false;

    {
        ScopeGuardian guard;
        guard.onCleanup([&cleanupExecuted]()
        {
            cleanupExecuted = true;
        });

        guard.dismiss();
    }

    EXPECT_FALSE(cleanupExecuted);
    EXPECT_TRUE(true);
}

TEST(ScopeGuardianTest, IsDismissedQuery)
{
    ScopeGuardian guard;

    EXPECT_FALSE(guard.isDismissed());
    guard.dismiss();
    EXPECT_TRUE(guard.isDismissed());
}

TEST(ScopeGuardianTest, CallbackCountQuery)
{
    ScopeGuardian guard;

    EXPECT_EQ(guard.callbackCount(), 0);

    guard.onCleanup([]()
    {
    });
    EXPECT_EQ(guard.callbackCount(), 1);

    guard.onSuccess([]()
    {
    });
    EXPECT_EQ(guard.callbackCount(), 2);

    guard.onFailure([]()
    {
    });
    EXPECT_EQ(guard.callbackCount(), 3);
}

TEST(ScopeGuardianTest, ResourceManagementExample)
{
    bool fileOpened       = false;
    bool fileClosed       = false;
    bool connectionOpened = false;
    bool connectionClosed = false;

    {
        ScopeGuardian guard;

        fileOpened = true;
        guard.onCleanup([&fileClosed]()
        {
            fileClosed = true;
        }, ScopeGuardian::CleanupTiming::Always, "close_file");

        connectionOpened = true;
        guard.onCleanup([&connectionClosed]()
        {
            connectionClosed = true;
        }, ScopeGuardian::CleanupTiming::Always, "close_connection");
    }

    EXPECT_TRUE(fileOpened);
    EXPECT_TRUE(fileClosed);
    EXPECT_TRUE(connectionOpened);
    EXPECT_TRUE(connectionClosed);
}

TEST(ScopeGuardianTest, ExceptionInCleanupCallback)
{
    bool firstCleanupExecuted  = false;
    bool secondCleanupExecuted = false;

    try
    {
        ScopeGuardian guard;

        guard.onCleanup([&firstCleanupExecuted]()
        {
            firstCleanupExecuted = true;
            throw std::runtime_error("Cleanup exception");
        }, ScopeGuardian::CleanupTiming::Always, "throwing_cleanup");

        guard.onCleanup([&secondCleanupExecuted]()
        {
            secondCleanupExecuted = true;
        }, ScopeGuardian::CleanupTiming::Always, "normal_cleanup");
    }
    catch (...)
    {
    }

    EXPECT_TRUE(firstCleanupExecuted);
    EXPECT_TRUE(secondCleanupExecuted);
    // Logger::Instance().shutdown();
}

TEST(ScopeGuardianTest, ThreadSafety)
{
    const int numThreads         = 10;
    const int callbacksPerThread = 100;
    std::atomic<int> totalExecutions{0};

    {
        ScopeGuardian guard;
        std::vector<std::thread> threads;

        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&guard, &totalExecutions]()
            {
                for (int j = 0; j < callbacksPerThread; ++j)
                {
                    guard.onCleanup([&totalExecutions]()
                    {
                        totalExecutions.fetch_add(1, std::memory_order_relaxed);
                    });
                }
            });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    // 所有回调都应该被执行
    EXPECT_EQ(totalExecutions.load(), numThreads * callbacksPerThread);
}

TEST(ScopeGuardianTest, ComplexScenarioWithMixedTimings)
{
    std::vector<std::string> executionLog;

    try
    {
        ScopeGuardian guard;

        guard.onCleanup([&executionLog]()
        {
            executionLog.push_back("always_1");
        }, ScopeGuardian::CleanupTiming::Always, "always_1");

        guard.onSuccess([&executionLog]()
        {
            executionLog.push_back("success_1");
        }, "success_1");

        guard.onFailure([&executionLog]()
        {
            executionLog.push_back("failure_1");
        }, "failure_1");

        guard.onCleanup([&executionLog]()
        {
            executionLog.push_back("always_2");
        }, ScopeGuardian::CleanupTiming::Always, "always_2");

        guard.onException([&executionLog](const std::exception_ptr&)
        {
            executionLog.push_back("exception_callback");
        });

        throw std::runtime_error("Test exception");
    }
    catch (...)
    {
        // 捕获异常以继续测试
    }

    // 验证执行顺序和内容
    EXPECT_TRUE(std::find(executionLog.begin(), executionLog.end(), "exception_callback") != executionLog.end());
    EXPECT_TRUE(std::find(executionLog.begin(), executionLog.end(), "always_1") != executionLog.end());
    EXPECT_TRUE(std::find(executionLog.begin(), executionLog.end(), "always_2") != executionLog.end());
    EXPECT_TRUE(std::find(executionLog.begin(), executionLog.end(), "failure_1") != executionLog.end());
    EXPECT_TRUE(std::find(executionLog.begin(), executionLog.end(), "success_1") == executionLog.end());
}

TEST(ScopeGuardianTest, RAIIPatternWithRealResources)
{
    // clang-format off
    class MockResource
    {
    public:
        MockResource(bool& created, bool& destroyed) : _created(created), _destroyed(destroyed) { _created = true; }
        ~MockResource() { _destroyed = true; }

        void use() { if (_destroyed) { throw std::runtime_error("Using destroyed resource"); } }

    private:
        bool& _created;
        bool& _destroyed;
    };
    // clang-format on

    bool resourceCreated   = false;
    bool resourceDestroyed = false;
    bool cleanupExecuted   = false;

    {
        auto resource = std::make_unique<MockResource>(resourceCreated, resourceDestroyed);

        ScopeGuardian guard;
        guard.onCleanup([&cleanupExecuted, &resource]()
        {
            cleanupExecuted = true;
            resource.reset();
        });

        resource->use();
    }

    EXPECT_TRUE(resourceCreated);
    EXPECT_TRUE(resourceDestroyed);
    EXPECT_TRUE(cleanupExecuted);
}

// ==================== 错误类型 ====================

TEST(ErrorIdTest, DefaultConstruction)
{
    ErrorId id;
    EXPECT_EQ(id.domain, 0);
    EXPECT_EQ(id.code, 0);
    EXPECT_EQ(id.value(), 0);
    EXPECT_TRUE(id.isSuccess());
}

TEST(ErrorIdTest, DomainCodeConstruction)
{
    ErrorId id(ErrorDomain::System, 0x1234);
    EXPECT_EQ(id.domain, static_cast<u16>(ErrorDomain::System));
    EXPECT_EQ(id.code, 0x1234);
    EXPECT_EQ(id.getDomain(), ErrorDomain::System);
    EXPECT_EQ(id.getCode(), 0x1234);
    EXPECT_FALSE(id.isSuccess());
}

TEST(ErrorIdTest, CombinedValueConstruction)
{
    u32 combined = 0x10011234; // Domain: 0x1001, Code: 0x1234
    ErrorId id(combined);
    EXPECT_EQ(id.domain, 0x1001);
    EXPECT_EQ(id.code, 0x1234);
    EXPECT_EQ(id.getDomain(), ErrorDomain::Core);
    EXPECT_EQ(id.getCode(), 0x1234);
}

TEST(ErrorIdTest, ValueConversion)
{
    ErrorId id(ErrorDomain::Graphics, 0x5678);
    u32 expected = (0x1003 << 16) | 0x5678; // 0x10035678
    EXPECT_EQ(id.value(), expected);
}

TEST(ErrorIdTest, HexFormatting)
{
    ErrorId id(ErrorDomain::Core, 0x1234);
    std::string hex = id.hex();
    EXPECT_EQ(hex, "10011234");
}

TEST(ErrorIdTest, StringFormatting)
{
    ErrorId id(ErrorDomain::System, 0x0042);
    std::string formatted = id.format();
    EXPECT_EQ(formatted, "System:0042");
}

TEST(ErrorIdTest, SuccessCheck)
{
    ErrorId success;
    EXPECT_TRUE(success.isSuccess());

    ErrorId error(ErrorDomain::System, 1);
    EXPECT_FALSE(error.isSuccess());
}

TEST(ErrorIdTest, Comparison)
{
    ErrorId id1(ErrorDomain::System, 0x1234);
    ErrorId id2(ErrorDomain::System, 0x1234);
    ErrorId id3(ErrorDomain::Core, 0x1234);
    ErrorId id4(ErrorDomain::System, 0x5678);

    EXPECT_EQ(id1, id2);
    EXPECT_NE(id1, id3);
    EXPECT_NE(id1, id4);

    EXPECT_TRUE(id1 < id3);
    EXPECT_TRUE(id1 < id4);
}

TEST(ErrorTest, DefaultConstruction)
{
    Error error;
    EXPECT_TRUE(error.isSuccess());
    EXPECT_FALSE(error.isError());
    EXPECT_FALSE(error);
    EXPECT_EQ(error.domain(), ErrorDomain::NoError);
    EXPECT_EQ(error.code(), 0);
}

TEST(ErrorTest, ErrorIdConstruction)
{
    ErrorId id(ErrorDomain::System, 0x1234);
    Error error(id);

    EXPECT_FALSE(error.isSuccess());
    EXPECT_TRUE(error.isError());
    EXPECT_TRUE(error);
    EXPECT_EQ(error.id(), id);
    EXPECT_EQ(error.domain(), ErrorDomain::System);
    EXPECT_EQ(error.code(), 0x1234);
}

TEST(ErrorTest, DomainCodeConstruction)
{
    Error error(ErrorDomain::Core, 0x5678);

    EXPECT_FALSE(error.isSuccess());
    EXPECT_TRUE(error.isError());
    EXPECT_EQ(error.domain(), ErrorDomain::Core);
    EXPECT_EQ(error.code(), 0x5678);
}

TEST(ErrorTest, EmptyMessageConstruction)
{
    Error error(ErrorDomain::Graphics, 0x1111);

    EXPECT_TRUE(error.isError());
    EXPECT_EQ(error.domain(), ErrorDomain::Graphics);
    EXPECT_EQ(error.code(), 0x1111);
}

TEST(ErrorTest, DomainCodeCheck)
{
    Error error(ErrorDomain::System, 0x1234);

    EXPECT_TRUE(error.isDomain(ErrorDomain::System));
    EXPECT_FALSE(error.isDomain(ErrorDomain::Core));

    EXPECT_TRUE(error.isCode(0x1234));
    EXPECT_FALSE(error.isCode(0x5678));
}

TEST(ErrorTest, Comparison)
{
    Error error1(ErrorDomain::System, 0x1234);
    Error error2(ErrorDomain::System, 0x1234);
    Error error3(ErrorDomain::Core, 0x1234);

    EXPECT_EQ(error1, error2);
    EXPECT_NE(error1, error3);
    EXPECT_TRUE(error1 < error3);
}

TEST(ResultTest, OkValueCreation)
{
    auto result1 = Ok(42);
    static_assert(std::is_same_v<decltype(result1), Result<int>>);
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), 42);

    auto result2 = Ok(std::string("hello"));
    static_assert(std::is_same_v<decltype(result2), Result<std::string>>);
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), "hello");

    std::string str = "world";
    auto result3    = Ok(std::move(str));
    EXPECT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value(), "world");
}

TEST(ResultTest, OkVoidCreation)
{
    auto result = Ok();
    static_assert(std::is_same_v<decltype(result), VResult>);
    EXPECT_TRUE(result.has_value());
}

TEST(ResultTest, ErrCreation)
{
    auto error1  = Err(ErrorDomain::System, 0x1234);
    auto result1 = Result<int>(error1);
    EXPECT_FALSE(result1.has_value());
    EXPECT_EQ(result1.error().domain(), ErrorDomain::System);
    EXPECT_EQ(result1.error().code(), 0x1234);

    ErrorId id(ErrorDomain::Core, 0x5678);
    auto error2  = Err(id);
    auto result2 = Result<std::string>(error2);
    EXPECT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error().id(), id);
}

TEST(ResultTest, ResultTypeDeduction)
{
    auto intResult = Ok(123);
    static_assert(std::is_same_v<decltype(intResult), Result<int>>);

    auto stringResult = Ok(std::string("test"));
    static_assert(std::is_same_v<decltype(stringResult), Result<std::string>>);

    auto voidResult = Ok();
    static_assert(std::is_same_v<decltype(voidResult), VResult>);
}

TEST(ResultTest, ResultChaining)
{
    // clang-format off
    auto getValue = [](bool success) -> Result<int>
    {
        if (success)
        { return Ok(42); }
        else
        { return Err(ErrorDomain::Core, 0x0001); }
    };
    // clang-format on

    auto doubleValue = [](int value) -> Result<int>
    {
        return Ok(value * 2);
    };

    auto result1 = getValue(true);
    EXPECT_TRUE(result1.has_value());
    if (result1)
    {
        auto result2 = doubleValue(result1.value());
        EXPECT_TRUE(result2.has_value());
        EXPECT_EQ(result2.value(), 84);
    }

    auto result3 = getValue(false);
    EXPECT_FALSE(result3.has_value());
    EXPECT_EQ(result3.error().domain(), ErrorDomain::Core);
}

TEST(ResultTest, VoidResultOperations)
{
    // clang-format off
    auto performOperation = [](bool success) -> VResult
    {
        if (success)
        { return Ok(); }
        else
        { return Err(ErrorDomain::System, 0x0002); }
    };
    // clang-format on

    auto result1 = performOperation(true);
    EXPECT_TRUE(result1.has_value());

    auto result2 = performOperation(false);
    EXPECT_FALSE(result2.has_value());
}

TEST(ResultTest, ComplexTypes)
{
    struct TestStruct
    {
        int value;
        std::string name;

        bool operator==(const TestStruct& other) const
        {
            return value == other.value && name == other.name;
        }
    };

    TestStruct data{42, "test"};
    auto result = Ok(data);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().value, 42);
    EXPECT_EQ(result.value().name, "test");
}

TEST(ResultTest, ErrorPropagation)
{
    auto step1 = []() -> Result<int>
    {
        return Err(ErrorDomain::System, 0x1001);
    };

    auto step2 = [&]() -> Result<std::string>
    {
        auto result = step1();
        if (!result)
        {
            return std::unexpected{result.error()};
        }
        return Ok(std::to_string(result.value()));
    };

    auto finalResult = step2();
    EXPECT_FALSE(finalResult.has_value());
    EXPECT_EQ(finalResult.error().domain(), ErrorDomain::System);
    EXPECT_EQ(finalResult.error().code(), 0x1001);
}

TEST(MacroTest, BEE_TRY_Success)
{
    auto getValue = [](int input) -> Result<int>
    {
        if (input > 0)
        {
            return Ok(input * 2);
        }
        return Err(ErrorDomain::Core, 0x0001);
    };

    auto processValue = [&](int input) -> Result<int>
    {
        int value = BEE_TRY(getValue(input));
        return Ok(value + 10);
    };

    auto result = processValue(5);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 20);
}

TEST(MacroTest, BEE_TRY_Failure)
{
    auto getValue = [](int input) -> Result<int>
    {
        if (input > 0)
        {
            return Ok(input * 2);
        }
        return Err(ErrorDomain::Core, 0x0001);
    };

    auto processValue = [&](int input) -> Result<int>
    {
        int value = BEE_TRY(getValue(input));
        return Ok(value + 10);
    };

    auto result = processValue(-1);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().domain(), ErrorDomain::Core);
    EXPECT_EQ(result.error().code(), 0x0001);
}

TEST(MacroTest, BEE_TRY_VOID_Success)
{
    auto performOperation = [](bool success) -> VResult
    {
        if (success)
        {
            return Ok();
        }
        return Err(ErrorDomain::System, 0x0002);
    };

    auto chainedOperation = [&](bool success) -> VResult
    {
        BEE_TRY_VOID(performOperation(success));
        return Ok();
    };

    auto result = chainedOperation(true);
    EXPECT_TRUE(result.has_value());
}

TEST(MacroTest, BEE_TRY_VOID_Failure)
{
    auto performOperation = [](bool success) -> VResult
    {
        if (success)
        {
            return Ok();
        }
        return Err(ErrorDomain::System, 0x0002);
    };

    auto chainedOperation = [&](bool success) -> VResult
    {
        BEE_TRY_VOID(performOperation(success));
        return Ok();
    };

    auto result = chainedOperation(false);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().domain(), ErrorDomain::System);
    EXPECT_EQ(result.error().code(), 0x0002);
}

TEST(MacroTest, BEE_TRY_OR_Success)
{
    auto getValue = [](int input) -> Result<int>
    {
        if (input > 0)
        {
            return Ok(input * 2);
        }
        return Err(ErrorDomain::Core, 0x0001);
    };

    auto processValue = [&](int input) -> Result<int>
    {
        int value = BEE_TRY_OR(getValue(input), 100);
        return Ok(value + 10);
    };

    auto result = processValue(5);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 20);
}

TEST(MacroTest, BEE_TRY_OR_Fallback)
{
    auto getValue = [](int input) -> Result<int>
    {
        if (input > 0)
        {
            return Ok(input * 2);
        }
        return Err(ErrorDomain::Core, 0x0001);
    };

    auto processValue = [&](int input) -> Result<int>
    {
        int value = BEE_TRY_OR(getValue(input), 100);
        return Ok(value + 10);
    };

    auto result = processValue(-1);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 110);
}

TEST(MacroTest, NestedBEE_TRY)
{
    auto step1 = [](int input) -> Result<int>
    {
        if (input > 0)
        {
            return Ok(input + 1);
        }
        return Err(ErrorDomain::Core, 0x0001);
    };

    auto step2 = [](int input) -> Result<int>
    {
        if (input > 5)
        {
            return Ok(input * 2);
        }
        return Err(ErrorDomain::Core, 0x0002);
    };

    auto step3 = [](int input) -> Result<int>
    {
        if (input > 12)
        {
            return Ok(input + 100);
        }
        return Err(ErrorDomain::Core, 0x0003);
    };

    auto processChain = [&](int input) -> Result<int>
    {
        int value1 = BEE_TRY(step1(input));
        int value2 = BEE_TRY(step2(value1));
        int value3 = BEE_TRY(step3(value2));
        return Ok(value3);
    };

    auto result1 = processChain(10);
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), 122);

    auto result2 = processChain(-1);
    EXPECT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error().code(), 0x0001);

    auto result3 = processChain(3);
    EXPECT_FALSE(result3.has_value());
    EXPECT_EQ(result3.error().code(), 0x0002);

    auto result4 = processChain(5);
    EXPECT_FALSE(result4.has_value());
    EXPECT_EQ(result4.error().code(), 0x0003);
}

TEST(MacroTest, MacroWithComplexExpressions)
{
    auto getVector = [](size_t size) -> Result<std::vector<int>>
    {
        if (size > 0 && size <= 10)
        {
            std::vector<int> vec(size);
            std::iota(vec.begin(), vec.end(), 1);
            return Ok(std::move(vec));
        }
        return Err(ErrorDomain::Core, 0x0004);
    };

    auto processVector = [&](size_t size) -> Result<int>
    {
        auto vec = BEE_TRY(getVector(size));
        int sum  = std::accumulate(vec.begin(), vec.end(), 0);
        return Ok(sum);
    };

    auto result = processVector(5);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 15);
}

TEST(MacroTest, MacroReturnTypeCompatibility)
{
    auto intFunction = [](bool success) -> Result<int>
    {
        return success ? Ok(42) : Err(ErrorDomain::Core, 0x0001);
    };

    auto stringFunction = [&](bool success) -> Result<std::string>
    {
        int value = BEE_TRY(intFunction(success));
        return Ok("Value: " + std::to_string(value));
    };

    auto voidFunction = [&](bool success) -> VResult
    {
        BEE_TRY_VOID(stringFunction(success));
        return Ok();
    };

    auto result1 = stringFunction(true);
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), "Value: 42");

    auto result2 = voidFunction(true);
    EXPECT_TRUE(result2.has_value());

    auto result3 = stringFunction(false);
    EXPECT_FALSE(result3.has_value());

    auto result4 = voidFunction(false);
    EXPECT_FALSE(result4.has_value());
}

TEST(TypeTraitsTest, IsResultType)
{
    static_assert(IsResult<Result<int>>);
    static_assert(IsResult<Result<std::string>>);
    static_assert(IsResult<VResult>);
    static_assert(!IsResult<int>);
    static_assert(!IsResult<std::string>);
    static_assert(!IsResult<Error>);

    static_assert(IsResult<Result<int>>);
    static_assert(IsResult<Result<std::string>>);
    static_assert(IsResult<VResult>);
    static_assert(!IsResult<int>);
    static_assert(!IsResult<std::string>);
    static_assert(!IsResult<Error>);
}

TEST(TypeTraitsTest, ResultValueType)
{
    static_assert(std::is_same_v<ResultValueType<Result<int>>, int>);
    static_assert(std::is_same_v<ResultValueType<Result<std::string>>, std::string>);
    static_assert(std::is_same_v<ResultValueType<VResult>, void>);

    static_assert(std::is_same_v<ResultValueType<Result<int>>, int>);
    static_assert(std::is_same_v<ResultValueType<Result<std::string>>, std::string>);
    static_assert(std::is_same_v<ResultValueType<VResult>, void>);
}

TEST(TypeTraitsTest, TypeDeductionInTemplates)
{
    auto processAnyResult = []<typename T>(const Result<T>& result) -> std::string
    {
        if constexpr (std::is_same_v<T, void>)
        {
            return result.has_value() ? "void success" : "void error";
        }
        else
        {
            return result.has_value() ? "value success" : "value error";
        }
    };

    auto intResult    = Ok(42);
    auto stringResult = Ok(std::string("test"));
    auto voidResult   = Ok();
    auto errorResult  = Result<int>(Err(ErrorDomain::Core, 0x0001));

    EXPECT_EQ(processAnyResult(intResult), "value success");
    EXPECT_EQ(processAnyResult(stringResult), "value success");
    EXPECT_EQ(processAnyResult(voidResult), "void success");
    EXPECT_EQ(processAnyResult(errorResult), "value error");
}

TEST(TypeTraitsTest, ConceptsLikeConstraints)
{
    auto processOnlyResults = []<typename T>(const T& result)
        -> std::enable_if_t<IsResult<T>, bool>
    {
        return result.has_value();
    };

    auto intResult   = Ok(42);
    auto errorResult = Result<int>(Err(ErrorDomain::Core, 0x0001));

    EXPECT_TRUE(processOnlyResults(intResult));
    EXPECT_FALSE(processOnlyResults(errorResult));
}

TEST(ErrorEdgeCaseTest, MaxErrorCodeValues)
{
    constexpr uint16_t maxCode = std::numeric_limits<uint16_t>::max();
    ErrorId id(ErrorDomain::System, maxCode);
    Error error(id);

    EXPECT_EQ(error.code(), maxCode);
    EXPECT_EQ(error.domain(), ErrorDomain::System);
}

TEST(ErrorEdgeCaseTest, ZeroErrorCode)
{
    ErrorId id(ErrorDomain::Core, 0);
    Error error(id);

    EXPECT_EQ(error.code(), 0);
    EXPECT_EQ(error.domain(), ErrorDomain::Core);
    EXPECT_FALSE(error.isSuccess());
}

TEST(ErrorPerformanceTest, ErrorCreationOverhead)
{
    constexpr int iterations = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        Error error(ErrorDomain::System, static_cast<uint16_t>(i % 65536));
        volatile auto domain = error.domain();
        (void)domain;
    }

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), iterations);
}

TEST(ErrorPerformanceTest, ResultCreationOverhead)
{
    constexpr int iterations = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        if (i % 2 == 0)
        {
            auto result         = Ok(i);
            volatile auto value = result.value();
            (void)value;
        }
        else
        {
            auto result            = Result<int>(Err(ErrorDomain::Core, 0x0001));
            volatile auto hasValue = result.has_value();
            (void)hasValue;
        }
    }

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), iterations * 2);
}

TEST(ErrorMemoryTest, ErrorSizeOptimization)
{
    static_assert(sizeof(Error) <= 64, "Error size should be optimized");
    static_assert(sizeof(ErrorId) <= 8, "ErrorId should be small");

    static_assert(sizeof(Result<int>) <= sizeof(std::expected<int, Error>), "Result should not have additional overhead");
    static_assert(sizeof(VResult) <= sizeof(std::expected<void, Error>), "RVoid should not have additional overhead");
}

TEST(ErrorCompatibilityTest, StdExpectedInteroperability)
{
    // 测试与 std::expected 的互操作性
    auto createStdExpected = [](bool success) -> std::expected<int, Error>
    {
        if (success)
        {
            return 42;
        }
        else
        {
            return std::unexpected{Error(ErrorDomain::Core, 0x0001)};
        }
    };

    auto convertToResult = [](const std::expected<int, Error>& expected) -> Result<int>
    {
        if (expected.has_value())
        {
            return Ok(expected.value());
        }
        else
        {
            return std::unexpected{expected.error()};
        }
    };

    auto stdResult1 = createStdExpected(true);
    auto ourResult1 = convertToResult(stdResult1);
    EXPECT_TRUE(ourResult1.has_value());
    EXPECT_EQ(ourResult1.value(), 42);

    auto stdResult2 = createStdExpected(false);
    auto ourResult2 = convertToResult(stdResult2);
    EXPECT_FALSE(ourResult2.has_value());
}
