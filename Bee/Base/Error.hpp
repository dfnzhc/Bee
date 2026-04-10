/**
 * @File Error.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/6/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Check.hpp"
#include "Config.hpp"
#include "Nameof.hpp"

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

// ============================================================================
// Severity
// ============================================================================

enum class Severity : u8
{
    Recoverable, // caller can handle
    Transient,   // temporary, may succeed on retry
    Fatal,       // cannot continue
    Bug,         // logic defect
};

BEE_ENUM_SCAN_RANGE(Severity, 0, 3, false)

[[nodiscard]] constexpr auto to_string(Severity s) noexcept -> std::string_view
{
    return enum_to_name(s);
}

// ============================================================================
// Error
// ============================================================================

struct Error
{
    std::string message;
    int errc                   = 0;
    Severity severity          = Severity::Recoverable;
    std::source_location where = std::source_location::current();

    #ifndef NDEBUG
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
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
// Construction utilities
// ============================================================================

[[nodiscard]] inline auto make_error(std::string message,
                                     Severity severity          = Severity::Recoverable,
                                     int errc                   = 0,
                                     std::source_location where = std::source_location::current()) -> Error
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
// Termination
// ============================================================================

/// Unrecoverable error: formats the Error, then delegates to check_fail -> Log(Fatal) -> abort.
[[noreturn]] void panic(Error e);

/// Returns the value if present; otherwise panics with the contained error.
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
// Exception-to-Result conversion
// ============================================================================

/// Calls fn inside try/catch, converting exceptions to Result<T>.
template <typename Fn>
[[nodiscard]] auto guard(Fn&& fn, std::string operation,
                         Severity exceptionSeverity = Severity::Fatal,
                         std::source_location where = std::source_location::current())
    -> Result<std::invoke_result_t<Fn>>
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

// ============================================================================
// Propagation macros
// ============================================================================

/// Evaluates EXPR (must return a Result<U>). If it holds an error, propagates immediately.
#define BEE_TRY(EXPR)                                                             \
    do {                                                                          \
        auto _bee_try_result_ = (EXPR);                                           \
        if (!_bee_try_result_) [[unlikely]] {                                     \
            return std::unexpected(std::move(_bee_try_result_.error()));          \
        }                                                                         \
    } while (false)

/// Evaluates EXPR (must return a Result<U>). On success, assigns to LHS. On error, propagates.
#define BEE_TRY_ASSIGN(LHS, EXPR)                                                 \
    do {                                                                          \
        auto _bee_try_result_ = (EXPR);                                           \
        if (!_bee_try_result_) [[unlikely]] {                                     \
            return std::unexpected(std::move(_bee_try_result_.error()));          \
        }                                                                         \
        (LHS) = std::move(_bee_try_result_.value());                              \
    } while (false)
