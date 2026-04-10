/**
 * @File ErrorTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/9
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include "Base/Diagnostics/Error.hpp"

#include <stdexcept>
#include <string>

using namespace bee;

// ============================================================================
// Severity
// ============================================================================

TEST(ErrorTest, SeverityToString)
{
    EXPECT_EQ(to_string(Severity::Recoverable), "Recoverable");
    EXPECT_EQ(to_string(Severity::Transient), "Transient");
    EXPECT_EQ(to_string(Severity::Fatal), "Fatal");
    EXPECT_EQ(to_string(Severity::Bug), "Bug");
}

TEST(ErrorTest, SeverityValuesDistinct)
{
    EXPECT_NE(static_cast<int>(Severity::Recoverable), static_cast<int>(Severity::Transient));
    EXPECT_NE(static_cast<int>(Severity::Transient), static_cast<int>(Severity::Fatal));
    EXPECT_NE(static_cast<int>(Severity::Fatal), static_cast<int>(Severity::Bug));
}

// ============================================================================
// Error construction
// ============================================================================

TEST(ErrorTest, MakeErrorDefaults)
{
    auto e = make_error("something failed");
    EXPECT_EQ(e.message, "something failed");
    EXPECT_EQ(e.errc, 0);
    EXPECT_EQ(e.severity, Severity::Recoverable);
}

TEST(ErrorTest, MakeErrorWithAllParams)
{
    auto e = make_error("timeout", Severity::Transient, 42);
    EXPECT_EQ(e.message, "timeout");
    EXPECT_EQ(e.errc, 42);
    EXPECT_EQ(e.severity, Severity::Transient);
}

TEST(ErrorTest, Retryable)
{
    EXPECT_TRUE(make_error("x", Severity::Transient).retryable());
    EXPECT_FALSE(make_error("x", Severity::Recoverable).retryable());
    EXPECT_FALSE(make_error("x", Severity::Fatal).retryable());
    EXPECT_FALSE(make_error("x", Severity::Bug).retryable());
}

TEST(ErrorTest, Unrecoverable)
{
    EXPECT_TRUE(make_error("x", Severity::Fatal).unrecoverable());
    EXPECT_TRUE(make_error("x", Severity::Bug).unrecoverable());
    EXPECT_FALSE(make_error("x", Severity::Recoverable).unrecoverable());
    EXPECT_FALSE(make_error("x", Severity::Transient).unrecoverable());
}

TEST(ErrorTest, FormatContainsCoreFields)
{
    auto e = make_error("disk full", Severity::Fatal, 28);
    auto s = e.format();
    EXPECT_NE(s.find("Fatal"), std::string::npos);
    EXPECT_NE(s.find("disk full"), std::string::npos);
    EXPECT_NE(s.find("errc=28"), std::string::npos);
    EXPECT_NE(s.find("ErrorTests.cpp"), std::string::npos);
}

// ============================================================================
// with_context
// ============================================================================

TEST(ErrorTest, WithContextDoesNotCrash)
{
    auto e = make_error("fail");
    e      = with_context(std::move(e), "key", "value");
    EXPECT_EQ(e.message, "fail");
    #ifndef NDEBUG
    ASSERT_EQ(e.context.size(), 1u);
    EXPECT_EQ(e.context[0].first, "key");
    EXPECT_EQ(e.context[0].second, "value");
    #endif
}

#ifndef NDEBUG
TEST(ErrorTest, WithContextAppearsInFormat)
{
    auto e = make_error("fail");
    e      = with_context(std::move(e), "user_id", "42");
    auto s = e.format();
    EXPECT_NE(s.find("user_id"), std::string::npos);
    EXPECT_NE(s.find("42"), std::string::npos);
}
#endif

// ============================================================================
// Result<T> / Status
// ============================================================================

TEST(ErrorTest, ResultWithValue)
{
    Result<int> r = 42;
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(*r, 42);
}

TEST(ErrorTest, ResultWithError)
{
    Result<int> r = std::unexpected(make_error("nope"));
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error().message, "nope");
}

TEST(ErrorTest, StatusSuccess)
{
    Status s = std::monostate{};
    EXPECT_TRUE(s.has_value());
}

TEST(ErrorTest, StatusFailure)
{
    Status s = std::unexpected(make_error("failed"));
    EXPECT_FALSE(s.has_value());
    EXPECT_EQ(s.error().message, "failed");
}

// ============================================================================
// BEE_TRY / BEE_TRY_ASSIGN
// ============================================================================

namespace
{

auto succeed_int() -> Result<int>
{
    return 42;
}

auto fail_int() -> Result<int>
{
    return std::unexpected(make_error("bad", Severity::Recoverable, 1));
}

auto succeed_void() -> Status
{
    return std::monostate{};
}

auto fail_void() -> Status
{
    return std::unexpected(make_error("bad void"));
}

auto try_chain_success() -> Result<int>
{
    BEE_TRY(succeed_void());
    int x = 0;
    BEE_TRY_ASSIGN(x, succeed_int());
    return x;
}

auto try_chain_fail_try() -> Result<int>
{
    BEE_TRY(fail_void());
    return 0; // should not reach
}

auto try_chain_fail_assign() -> Result<int>
{
    int x = 0;
    BEE_TRY_ASSIGN(x, fail_int());
    return x; // should not reach
}

} // anonymous namespace

TEST(ErrorTest, TrySuccess)
{
    auto r = try_chain_success();
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(*r, 42);
}

TEST(ErrorTest, TryPropagatesOnFailure)
{
    auto r = try_chain_fail_try();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error().message, "bad void");
}

TEST(ErrorTest, TryAssignPropagatesOnFailure)
{
    auto r = try_chain_fail_assign();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error().message, "bad");
    EXPECT_EQ(r.error().errc, 1);
}

// ============================================================================
// panic / value_or_panic
// ============================================================================

TEST(ErrorDeathTest, PanicAborts)
{
    EXPECT_DEATH(panic(make_error("everything is on fire", Severity::Fatal)), "everything is on fire");
}

TEST(ErrorTest, ValueOrPanicReturnsValue)
{
    Result<int> r = 99;
    EXPECT_EQ(value_or_panic(std::move(r)), 99);
}

TEST(ErrorDeathTest, ValueOrPanicAbortsOnError)
{
    Result<int> r = std::unexpected(make_error("no value"));
    EXPECT_DEATH((void)value_or_panic(std::move(r)), "no value");
}

// ============================================================================
// guard
// ============================================================================

TEST(ErrorTest, GuardWithNonThrowingLambda)
{
    auto r = guard([] {
        return 42;
    }, "compute");
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(*r, 42);
}

TEST(ErrorTest, GuardWithThrowingLambda)
{
    auto r = guard([]() -> int {
        throw std::runtime_error("oops");
    }, "compute");
    EXPECT_FALSE(r.has_value());
    EXPECT_NE(r.error().message.find("oops"), std::string::npos);
    EXPECT_NE(r.error().message.find("compute"), std::string::npos);
    EXPECT_EQ(r.error().severity, Severity::Fatal);
}

TEST(ErrorTest, GuardWithVoidLambda)
{
    auto r = guard([] {
    }, "noop");
    EXPECT_TRUE(r.has_value());
}

TEST(ErrorTest, GuardWithUnknownException)
{
    auto r = guard([]() -> int {
        throw 42;
    }, "compute");
    EXPECT_FALSE(r.has_value());
    EXPECT_NE(r.error().message.find("unknown exception"), std::string::npos);
}
