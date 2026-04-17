/**
 * @File Error.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/6/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Diagnostics/Check.hpp"
#include "Base/Core/Config.hpp"
#include "Base/Core/InlineString.hpp"
#include "Base/Reflection/Nameof.hpp"

#include <chrono>
#include <expected>
#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__cpp_lib_stacktrace)
    #include <stacktrace>
#endif

namespace bee
{

enum class Severity : u8
{
    Recoverable, // 调用方可处理
    Transient,   // 临时性问题，重试可能成功
    Fatal,       // 无法继续执行
    Bug,         // 逻辑缺陷
};

BEE_ENUM_DEFAULT_SETUP(Severity, 4);

// ============================================================================
// 错误对象
// ============================================================================

struct Error
{
    InlineString         message;
    int                  errc     = 0;
    Severity             severity = Severity::Recoverable;
    std::source_location where    = std::source_location::current();

    // Always present for ABI stability; populated only in debug builds.
    std::chrono::system_clock::time_point              timestamp{};
    std::vector<std::pair<InlineString, InlineString>> context{};
#if defined(__cpp_lib_stacktrace)
    std::optional<std::stacktrace> trace{};
#endif

    [[nodiscard]] auto retryable() const noexcept -> bool
    {
        return severity == Severity::Transient;
    }

    [[nodiscard]] auto unrecoverable() const noexcept -> bool
    {
        return severity == Severity::Fatal || severity == Severity::Bug;
    }

    [[nodiscard]] auto format() const -> std::string;
};

// ============================================================================
// Result<T> / Status
// ============================================================================

template <typename T>
using Result = std::expected<T, Error>;

using Status = Result<std::monostate>;

// ============================================================================
// 构造工具
// ============================================================================

[[nodiscard]] inline auto make_error(
    std::string_view     message,
    Severity             severity = Severity::Recoverable,
    int                  errc     = 0,
    std::source_location where    = std::source_location::current()
) -> Error
{
    Error e;
    e.message  = message;
    e.errc     = errc;
    e.severity = severity;
    e.where    = where;
#ifndef NDEBUG
    e.timestamp = std::chrono::system_clock::now();
    if (severity == Severity::Fatal || severity == Severity::Bug) {
    #if defined(__cpp_lib_stacktrace)
        e.trace = std::stacktrace::current();
    #endif
    }
#endif
    return e;
}

[[nodiscard]] inline auto with_context(Error e, std::string_view key, std::string_view value) -> Error
{
#ifndef NDEBUG
    e.context.emplace_back(InlineString(key), InlineString(value));
#else
    (void)key;
    (void)value;
#endif
    return e;
}

// ============================================================================
// 终止路径
// ============================================================================

/// 不可恢复错误：格式化 Error 后，委托给 check_fail -> Log(Fatal) -> abort。
[[noreturn]] void panic(Error e);

/// 若结果中有值则返回；否则使用其中携带的错误触发 panic。
template <typename T>
[[nodiscard]] auto value_or_panic(Result<T>&& result, std::source_location loc = std::source_location::current()) -> T
{
    if (result) [[likely]] {
        return std::move(*result);
    }
    auto err  = std::move(result.error());
    err.where = loc;
    panic(std::move(err));
}

// ============================================================================
// 异常到 Result 的转换
// ============================================================================

/// 在 try/catch 中调用 fn，并将异常转换为 Result<T>。
template <typename Fn>
[[nodiscard]] auto guard(
    Fn&&                 fn,
    std::string_view     operation,
    Severity             exceptionSeverity = Severity::Fatal,
    std::source_location where             = std::source_location::current()
) -> Result<std::invoke_result_t<Fn>>
{
    using R = std::invoke_result_t<Fn>;
    static_assert(!std::is_reference_v<R>, "guard() does not support reference returns");

    try {
        if constexpr (std::is_void_v<R>) {
            std::forward<Fn>(fn)();
            return {};
        } else {
            return std::forward<Fn>(fn)();
        }
    } catch (const std::exception& ex) {
        return std::unexpected(make_error(std::format("{}: {}", operation, ex.what()), exceptionSeverity, 0, where));
    } catch (...) {
        return std::unexpected(make_error(std::format("{}: unknown exception", operation), exceptionSeverity, 0, where));
    }
}

} // namespace bee

/// 计算 EXPR（必须返回 Result<U>）。若包含错误则立即向上传播。
#define BEE_TRY(EXPR)                                                    \
    do {                                                                 \
        auto _bee_try_result_ = (EXPR);                                  \
        if (!_bee_try_result_) [[unlikely]] {                            \
            return std::unexpected(std::move(_bee_try_result_.error())); \
        }                                                                \
    } while (false)

/// 计算 EXPR（必须返回 Result<U>）。成功时赋值给 LHS；失败时向上传播。
#define BEE_TRY_ASSIGN(LHS, EXPR)                                        \
    do {                                                                 \
        auto _bee_try_result_ = (EXPR);                                  \
        if (!_bee_try_result_) [[unlikely]] {                            \
            return std::unexpected(std::move(_bee_try_result_.error())); \
        }                                                                \
        (LHS) = std::move(_bee_try_result_.value());                     \
    } while (false)

/// 计算 EXPR（必须返回 Result<U>）。若包含错误则附加上下文后向上传播。
#define BEE_TRY_CTX(EXPR, KEY, VALUE)                                                                     \
    do {                                                                                                  \
        auto _bee_try_result_ = (EXPR);                                                                   \
        if (!_bee_try_result_) [[unlikely]] {                                                             \
            return std::unexpected(::bee::with_context(std::move(_bee_try_result_.error()), (KEY), (VALUE))); \
        }                                                                                                 \
    } while (false)

/// 计算 EXPR。成功时赋值给 LHS；失败时附加上下文后向上传播。
#define BEE_TRY_ASSIGN_CTX(LHS, EXPR, KEY, VALUE)                                                        \
    do {                                                                                                  \
        auto _bee_try_result_ = (EXPR);                                                                   \
        if (!_bee_try_result_) [[unlikely]] {                                                             \
            return std::unexpected(::bee::with_context(std::move(_bee_try_result_.error()), (KEY), (VALUE))); \
        }                                                                                                 \
        (LHS) = std::move(_bee_try_result_.value());                                                      \
    } while (false)

/// 计算 EXPR（必须返回 Result<U>）。若包含错误则记录日志并继续执行。
#define BEE_LOG_RESULT(EXPR, MSG)                                                         \
    do {                                                                                  \
        auto _bee_lr_ = (EXPR);                                                           \
        if (!_bee_lr_) [[unlikely]] {                                                     \
            auto _bee_lr_msg_ = std::format("{}: {}", (MSG), _bee_lr_.error().format());  \
            ::bee::LogRaw(::bee::LogLevel::Error, "Result", _bee_lr_msg_);                \
        }                                                                                 \
    } while (false)
