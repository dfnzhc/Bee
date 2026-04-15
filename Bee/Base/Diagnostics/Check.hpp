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
#include <sstream>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>

namespace bee
{

/// 断言/检查失败时调用的函数指针：在日志记录之后、进程中止之前执行。
/// 接收失败表达式字符串、可选的详细消息以及源码位置信息。
using FailureHandler = void (*)(std::string_view expr, std::string_view message, std::source_location loc) noexcept;

// --- 控制接口 ---

void                         SetFailureHandler(FailureHandler handler) noexcept;
[[nodiscard]] FailureHandler GetFailureHandler() noexcept;

// --- 内部失败路径 ---

namespace detail
{
    extern std::atomic<FailureHandler> g_failure_handler;

    /// 核心失败函数：记录上下文日志、调用处理器、触发调试中断（仅调试构建）、然后中止进程。
    [[noreturn]] void check_fail(std::string_view check_type, std::string_view expr, std::string_view message, std::source_location loc);

    /// 比较失败：格式化两侧值后，委托给 check_fail。
    template <typename A, typename B>
    [[noreturn]] void check_op_fail(
            const A&             a,
            const B&             b,
            std::string_view     expr_a,
            std::string_view     expr_b,
            std::string_view     op_str,
            std::string_view     check_type,
            std::source_location loc
    )
    {
        auto full_expr = std::format("{} {} {}", expr_a, op_str, expr_b);

        std::string values;
        if constexpr (requires(std::ostream& os, const std::remove_cvref_t<A>& lhs, const std::remove_cvref_t<B>& rhs) {
                          os << lhs;
                          os << rhs;
                      }) {
            try {
                std::ostringstream oss;
                oss << expr_a << " = " << a << ", " << expr_b << " = " << b;
                values = oss.str();
            } catch (...) {
                values = "value formatting failed";
            }
        } else {
            values = "values unavailable (types are not stream-insertable)";
        }

        check_fail(check_type, full_expr, values, loc);
    }

} // namespace detail
} // namespace bee

// ============================================================================
// 常驻检查（所有构建配置均启用）
// ============================================================================

#define BEE_CHECK(expr)                                                                     \
    do {                                                                                    \
        if (BEE_UNLIKELY(!(expr)))                                                          \
            ::bee::detail::check_fail("Check", #expr, "", std::source_location::current()); \
    } while (0)

#define BEE_CHECK_MSG(expr, msg)                                                               \
    do {                                                                                       \
        if (BEE_UNLIKELY(!(expr)))                                                             \
            ::bee::detail::check_fail("Check", #expr, (msg), std::source_location::current()); \
    } while (0)

// --- 比较检查辅助宏（内部） ---

#define BEE_DETAIL_CHECK_OP(a, b, op, op_str, check_type)                                                                      \
    do {                                                                                                                       \
        auto&& bee_lhs_ = (a);                                                                                                 \
        auto&& bee_rhs_ = (b);                                                                                                 \
        if (BEE_UNLIKELY(!(bee_lhs_ op bee_rhs_)))                                                                             \
            ::bee::detail::check_op_fail(bee_lhs_, bee_rhs_, #a, #b, (op_str), (check_type), std::source_location::current()); \
    } while (0)

#define BEE_CHECK_EQ(a, b) BEE_DETAIL_CHECK_OP(a, b, ==, "==", "Check")
#define BEE_CHECK_NE(a, b) BEE_DETAIL_CHECK_OP(a, b, !=, "!=", "Check")
#define BEE_CHECK_LT(a, b) BEE_DETAIL_CHECK_OP(a, b, <, "<", "Check")
#define BEE_CHECK_LE(a, b) BEE_DETAIL_CHECK_OP(a, b, <=, "<=", "Check")
#define BEE_CHECK_GT(a, b) BEE_DETAIL_CHECK_OP(a, b, >, ">", "Check")
#define BEE_CHECK_GE(a, b) BEE_DETAIL_CHECK_OP(a, b, >=, ">=", "Check")

// ============================================================================
// 仅调试断言（在 release / NDEBUG 构建中剔除）
// ============================================================================

#ifndef NDEBUG

    #define BEE_ASSERT(expr)                                                                        \
        do {                                                                                        \
            if (BEE_UNLIKELY(!(expr)))                                                              \
                ::bee::detail::check_fail("Assertion", #expr, "", std::source_location::current()); \
        } while (0)

    #define BEE_ASSERT_MSG(expr, msg)                                                                  \
        do {                                                                                           \
            if (BEE_UNLIKELY(!(expr)))                                                                 \
                ::bee::detail::check_fail("Assertion", #expr, (msg), std::source_location::current()); \
        } while (0)

    #define BEE_ASSERT_EQ(a, b) BEE_DETAIL_CHECK_OP(a, b, ==, "==", "Assertion")
    #define BEE_ASSERT_NE(a, b) BEE_DETAIL_CHECK_OP(a, b, !=, "!=", "Assertion")
    #define BEE_ASSERT_LT(a, b) BEE_DETAIL_CHECK_OP(a, b, <, "<", "Assertion")
    #define BEE_ASSERT_LE(a, b) BEE_DETAIL_CHECK_OP(a, b, <=, "<=", "Assertion")
    #define BEE_ASSERT_GT(a, b) BEE_DETAIL_CHECK_OP(a, b, >, ">", "Assertion")
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
// 不可达代码标记
// ============================================================================

#ifndef NDEBUG
    #define BEE_UNREACHABLE()                                                                                   \
        do {                                                                                                    \
            ::bee::detail::check_fail("Unreachable", "BEE_UNREACHABLE()", "", std::source_location::current()); \
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
