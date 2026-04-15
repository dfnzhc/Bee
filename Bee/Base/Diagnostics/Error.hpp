/**
 * @File Error.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/6/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Diagnostics/Check.hpp"
#include "Base/Core/Config.hpp"
#include "Base/Reflection/Nameof.hpp"

#include <chrono>
#include <expected>
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
    std::string          message;
    int                  errc     = 0;
    Severity             severity = Severity::Recoverable;
    std::source_location where    = std::source_location::current();

#ifndef NDEBUG
    std::chrono::system_clock::time_point            timestamp = std::chrono::system_clock::now();
    std::vector<std::pair<std::string, std::string>> context{};
    #if defined(__cpp_lib_stacktrace)
    std::optional<std::stacktrace> trace{};
    #endif
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
    std::string          message,
    Severity             severity = Severity::Recoverable,
    int                  errc     = 0,
    std::source_location where    = std::source_location::current()
) -> Error
{
    Error e;
    e.message  = std::move(message);
    e.errc     = errc;
    e.severity = severity;
    e.where    = where;
#ifndef NDEBUG
    if (severity == Severity::Fatal || severity == Severity::Bug) {
    #if defined(__cpp_lib_stacktrace)
        e.trace = std::stacktrace::current();
    #endif
    }
#endif
    return e;
}

[[nodiscard]] inline auto with_context(Error e, std::string key, std::string value) -> Error
{
#ifndef NDEBUG
    e.context.emplace_back(std::move(key), std::move(value));
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
    std::string          operation,
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
        return std::unexpected(make_error(operation + ": " + ex.what(), exceptionSeverity, 0, where));
    } catch (...) {
        return std::unexpected(make_error(operation + ": unknown exception", exceptionSeverity, 0, where));
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
