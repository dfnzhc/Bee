/**
 * @File Log.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/9
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Core/Config.hpp"
#include "Base/Reflection/Enum.hpp"

#include <array>
#include <atomic>
#include <format>
#include <source_location>
#include <string>
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

BEE_ENUM_DEFAULT_SETUP(LogLevel, 6);

using LogSink = void (*)(LogLevel level, std::string_view category, std::string_view message, std::source_location location);

// --- Control interface ---

void                   set_log_sink(LogSink sink) noexcept;
void                   set_log_level(LogLevel level) noexcept;
[[nodiscard]] LogSink  get_log_sink() noexcept;
[[nodiscard]] LogLevel get_log_level() noexcept;
void                   enable_default_logging(LogLevel level = LogLevel::Info) noexcept;

// --- Built-in sink ---

void default_console_sink(LogLevel level, std::string_view category, std::string_view message, std::source_location location);

// --- Atomic state (extern, defined in Log.cpp) ---

namespace detail
{
    extern std::atomic<LogSink>  gLogSink;
    extern std::atomic<LogLevel> gLogLevel;

    /// Fast-path check: returns the sink if logging should proceed, nullptr otherwise.
    inline auto acquire_sink(LogLevel level) noexcept -> LogSink
    {
        if (level < gLogLevel.load(std::memory_order_relaxed))
            return nullptr;
        return gLogSink.load(std::memory_order_acquire);
    }

    /// Stack-buffered formatting threshold.
    static constexpr std::size_t kLogStackBufSize = 256;

} // namespace detail

// --- FormatWithLocation: resolves source_location + variadic template cooperation ---

template <typename... Args>
struct FormatWithLocation
{
    std::format_string<Args...> fmt;
    std::source_location        loc;

    template <typename T>
    consteval FormatWithLocation(T&& f, std::source_location l = std::source_location::current())
        : fmt(std::forward<T>(f))
        , loc(l)
    {
    }
};

// --- LogRaw: pre-formatted message pass-through ---

inline void LogRaw(LogLevel level, std::string_view category, std::string_view message, std::source_location loc = std::source_location::current())
{
    auto sink = detail::acquire_sink(level);
    if (!sink)
        return;
    sink(level, category, message, loc);
}

// --- Log: std::format style with stack-buffer optimization ---

template <typename... Args>
void Log(LogLevel level, std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto sink = detail::acquire_sink(level);
    if (!sink)
        return;

    std::array<char, detail::kLogStackBufSize> buf;
    auto result = std::format_to_n(buf.data(), buf.size(), fmt_loc.fmt, std::forward<Args>(args)...);
    if (static_cast<std::size_t>(result.size) <= buf.size()) {
        sink(level, category, std::string_view(buf.data(), static_cast<std::size_t>(result.size)), fmt_loc.loc);
    } else {
        auto heap_msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
        sink(level, category, heap_msg, fmt_loc.loc);
    }
}

// --- Convenience functions ---

namespace detail
{
    /// Shared format+dispatch: formats into stack buffer, falls back to heap.
    template <typename... Args>
    void log_dispatch(LogSink sink, LogLevel level, std::string_view category,
                      std::format_string<Args...> fmt, std::source_location loc, Args&&... args)
    {
        std::array<char, kLogStackBufSize> buf;
        auto result = std::format_to_n(buf.data(), buf.size(), fmt, std::forward<Args>(args)...);
        if (static_cast<std::size_t>(result.size) <= buf.size()) {
            sink(level, category, std::string_view(buf.data(), static_cast<std::size_t>(result.size)), loc);
        } else {
            auto heap_msg = std::format(fmt, std::forward<Args>(args)...);
            sink(level, category, heap_msg, loc);
        }
    }
} // namespace detail

template <typename... Args>
void LogTrace(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto sink = detail::acquire_sink(LogLevel::Trace);
    if (!sink)
        return;
    detail::log_dispatch(sink, LogLevel::Trace, category, fmt_loc.fmt, fmt_loc.loc, std::forward<Args>(args)...);
}

template <typename... Args>
void LogDebug(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto sink = detail::acquire_sink(LogLevel::Debug);
    if (!sink)
        return;
    detail::log_dispatch(sink, LogLevel::Debug, category, fmt_loc.fmt, fmt_loc.loc, std::forward<Args>(args)...);
}

template <typename... Args>
void LogInfo(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto sink = detail::acquire_sink(LogLevel::Info);
    if (!sink)
        return;
    detail::log_dispatch(sink, LogLevel::Info, category, fmt_loc.fmt, fmt_loc.loc, std::forward<Args>(args)...);
}

template <typename... Args>
void LogWarn(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto sink = detail::acquire_sink(LogLevel::Warn);
    if (!sink)
        return;
    detail::log_dispatch(sink, LogLevel::Warn, category, fmt_loc.fmt, fmt_loc.loc, std::forward<Args>(args)...);
}

template <typename... Args>
void LogError(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto sink = detail::acquire_sink(LogLevel::Error);
    if (!sink)
        return;
    detail::log_dispatch(sink, LogLevel::Error, category, fmt_loc.fmt, fmt_loc.loc, std::forward<Args>(args)...);
}

template <typename... Args>
void LogFatal(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    auto sink = detail::acquire_sink(LogLevel::Fatal);
    if (!sink)
        return;
    detail::log_dispatch(sink, LogLevel::Fatal, category, fmt_loc.fmt, fmt_loc.loc, std::forward<Args>(args)...);
}

} // namespace bee
