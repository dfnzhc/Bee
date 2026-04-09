/**
 * @File Log.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/9
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Config.hpp"
#include "Nameof.hpp"

#include <atomic>
#include <format>
#include <source_location>
#include <string_view>
#include <type_traits>

namespace bee
{

enum class LogLevel : u8
{
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

using LogSink = void (*)(LogLevel level, std::string_view category, std::string_view message, std::source_location location);

// --- Control API ---

void SetLogSink(LogSink sink) noexcept;
void SetLogLevel(LogLevel level) noexcept;
[[nodiscard]] LogSink GetLogSink() noexcept;
[[nodiscard]] LogLevel GetLogLevel() noexcept;
void EnableDefaultLogging(LogLevel level = LogLevel::Info) noexcept;

[[nodiscard]] constexpr auto LogLevelToString(LogLevel level) noexcept -> std::string_view
{
    return enum_to_name(level);
}

// --- Built-in sink ---

void DefaultConsoleSink(LogLevel level, std::string_view category, std::string_view message, std::source_location location);

// --- Atomic state (extern, defined in Log.cpp) ---

namespace detail
{
    extern std::atomic<LogSink> g_log_sink;
    extern std::atomic<LogLevel> g_log_level;
} // namespace detail

// --- LogRaw: pre-formatted message ---

inline void LogRaw(LogLevel level, std::string_view category, std::string_view message,
                   std::source_location loc = std::source_location::current())
{
    if (static_cast<u8>(level) < static_cast<u8>(detail::g_log_level.load(std::memory_order_relaxed)))
        return;

    auto sink = detail::g_log_sink.load(std::memory_order_acquire);
    if (!sink)
        return;

    sink(level, category, message, loc);
}

// --- FormatWithLocation: solves source_location + variadic template ---

template <typename... Args>
struct FormatWithLocation
{
    std::format_string<Args...> fmt;
    std::source_location loc;

    template <typename T>
    consteval FormatWithLocation(T&& f, std::source_location l = std::source_location::current())
        : fmt(std::forward<T>(f))
        , loc(l)
    {
    }
};

// --- Log: std::format style ---

template <typename... Args>
void Log(LogLevel level, std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    if (static_cast<u8>(level) < static_cast<u8>(detail::g_log_level.load(std::memory_order_relaxed)))
        return;

    auto sink = detail::g_log_sink.load(std::memory_order_acquire);
    if (!sink)
        return;

    auto message = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    sink(level, category, message, fmt_loc.loc);
}

// --- Convenience functions ---

template <typename... Args>
void LogTrace(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    LogRaw(LogLevel::Trace, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogDebug(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    LogRaw(LogLevel::Debug, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogInfo(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    LogRaw(LogLevel::Info, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogWarn(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    LogRaw(LogLevel::Warn, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogError(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    LogRaw(LogLevel::Error, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogFatal(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    LogRaw(LogLevel::Fatal, category, msg, fmt_loc.loc);
}

} // namespace bee
