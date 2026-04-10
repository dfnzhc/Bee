/**
 * @File Check.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/9
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Core/Defines.hpp"
#include "Base/Diagnostics/Log.hpp"

#include <atomic>
#include <format>
#include <source_location>
#include <string_view>

namespace bee
{

/// Function pointer called on assertion/check failure, after logging, before abort.
/// Receives the failed expression string, an optional detail message, and the source location.
using FailureHandler = void (*)(std::string_view expr, std::string_view message, std::source_location loc) noexcept;

// --- Control API ---

void SetFailureHandler(FailureHandler handler) noexcept;
[[nodiscard]] FailureHandler GetFailureHandler() noexcept;

// --- Internal failure path ---

namespace detail
{

    extern std::atomic<FailureHandler> g_failure_handler;

    /// Core failure function: logs context, calls handler, debug-breaks (debug only), aborts.
    [[noreturn]] void check_fail(std::string_view check_type, std::string_view expr, std::string_view message, std::source_location loc);

    /// Comparison failure: formats both values, then delegates to check_fail.
    template <typename A, typename B>
    [[noreturn]] void check_op_fail(const A& a, const B& b, std::string_view expr_a, std::string_view expr_b, std::string_view op_str,
                                    std::string_view check_type, std::source_location loc)
    {
        auto full_expr = std::format("{} {} {}", expr_a, op_str, expr_b);
        auto values    = std::format("{} = {}, {} = {}", expr_a, a, expr_b, b);
        check_fail(check_type, full_expr, values, loc);
    }

} // namespace detail
} // namespace bee

// ============================================================================
// Always-on checks (active in all build configurations)
// ============================================================================

#define BEE_CHECK(expr)                                                          \
    do {                                                                         \
        if (BEE_UNLIKELY(!(expr)))                                               \
            ::bee::detail::check_fail("Check", #expr, "",                        \
                                      std::source_location::current());          \
    } while (0)

#define BEE_CHECK_MSG(expr, msg)                                                 \
    do {                                                                         \
        if (BEE_UNLIKELY(!(expr)))                                               \
            ::bee::detail::check_fail("Check", #expr, (msg),                     \
                                      std::source_location::current());          \
    } while (0)

// --- Comparison check helper (internal) ---

#define BEE_DETAIL_CHECK_OP(a, b, op, op_str, check_type)                        \
    do {                                                                         \
        auto&& bee_lhs_ = (a);                                                   \
        auto&& bee_rhs_ = (b);                                                   \
        if (BEE_UNLIKELY(!(bee_lhs_ op bee_rhs_)))                               \
            ::bee::detail::check_op_fail(bee_lhs_, bee_rhs_, #a, #b, (op_str),   \
                                         (check_type),                           \
                                         std::source_location::current());       \
    } while (0)

#define BEE_CHECK_EQ(a, b) BEE_DETAIL_CHECK_OP(a, b, ==, "==", "Check")
#define BEE_CHECK_NE(a, b) BEE_DETAIL_CHECK_OP(a, b, !=, "!=", "Check")
#define BEE_CHECK_LT(a, b) BEE_DETAIL_CHECK_OP(a, b, <,  "<",  "Check")
#define BEE_CHECK_LE(a, b) BEE_DETAIL_CHECK_OP(a, b, <=, "<=", "Check")
#define BEE_CHECK_GT(a, b) BEE_DETAIL_CHECK_OP(a, b, >,  ">",  "Check")
#define BEE_CHECK_GE(a, b) BEE_DETAIL_CHECK_OP(a, b, >=, ">=", "Check")

// ============================================================================
// Debug-only assertions (stripped in release / NDEBUG builds)
// ============================================================================

#ifndef NDEBUG

#define BEE_ASSERT(expr)                                                         \
        do {                                                                     \
            if (BEE_UNLIKELY(!(expr)))                                           \
                ::bee::detail::check_fail("Assertion", #expr, "",                \
                                          std::source_location::current());      \
        } while (0)

#define BEE_ASSERT_MSG(expr, msg)                                                \
        do {                                                                     \
            if (BEE_UNLIKELY(!(expr)))                                           \
                ::bee::detail::check_fail("Assertion", #expr, (msg),             \
                                          std::source_location::current());      \
        } while (0)

#define BEE_ASSERT_EQ(a, b) BEE_DETAIL_CHECK_OP(a, b, ==, "==", "Assertion")
#define BEE_ASSERT_NE(a, b) BEE_DETAIL_CHECK_OP(a, b, !=, "!=", "Assertion")
#define BEE_ASSERT_LT(a, b) BEE_DETAIL_CHECK_OP(a, b, <,  "<",  "Assertion")
#define BEE_ASSERT_LE(a, b) BEE_DETAIL_CHECK_OP(a, b, <=, "<=", "Assertion")
#define BEE_ASSERT_GT(a, b) BEE_DETAIL_CHECK_OP(a, b, >,  ">",  "Assertion")
#define BEE_ASSERT_GE(a, b) BEE_DETAIL_CHECK_OP(a, b, >=, ">=", "Assertion")

#else

#define BEE_ASSERT(expr)          ((void)0)
#define BEE_ASSERT_MSG(expr, msg) ((void)0)
#define BEE_ASSERT_EQ(a, b)       ((void)0)
#define BEE_ASSERT_NE(a, b)       ((void)0)
#define BEE_ASSERT_LT(a, b)       ((void)0)
#define BEE_ASSERT_LE(a, b)       ((void)0)
#define BEE_ASSERT_GT(a, b)       ((void)0)
#define BEE_ASSERT_GE(a, b)       ((void)0)

#endif

// ============================================================================
// Unreachable code marker
// ============================================================================

#ifndef NDEBUG
#define BEE_UNREACHABLE()                                                        \
        do {                                                                     \
            ::bee::detail::check_fail("Unreachable", "BEE_UNREACHABLE()", "",    \
                                      std::source_location::current());          \
        } while (0)
#else
#if defined(_MSC_VER)
#define BEE_UNREACHABLE() __assume(false)
#elif BEE_HAS_BUILTIN(__builtin_unreachable)
#define BEE_UNREACHABLE() __builtin_unreachable()
#else
#define BEE_UNREACHABLE() ((void)0)
#endif
#endif
