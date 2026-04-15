/**
 * @File Log.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/9
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Core/Config.hpp"
#include "Base/Reflection/Enum.hpp"

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

BEE_ENUM_DEFAULT_SETUP(LogLevel, 6);

using LogSink = void (*)(LogLevel level, std::string_view category, std::string_view message, std::source_location location);

// --- 控制接口 ---

void                   set_log_sink(LogSink sink) noexcept;
void                   set_log_level(LogLevel level) noexcept;
[[nodiscard]] LogSink  get_log_sink() noexcept;
[[nodiscard]] LogLevel get_log_level() noexcept;
void                   enable_default_logging(LogLevel level = LogLevel::Info) noexcept;

// --- 内置输出端 ---

void default_console_sink(LogLevel level, std::string_view category, std::string_view message, std::source_location location);

// --- 原子状态（extern，在 Log.cpp 中定义） ---

namespace detail
{
    extern std::atomic<LogSink>  gLogSink;
    extern std::atomic<LogLevel> gLogLevel;
} // namespace detail

// --- FormatWithLocation：解决 source_location 与可变参数模板协同问题 ---

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

// 日志接口使用 Pascal 命名用于区分一般调用

// --- LogRaw：已格式化消息直传 ---

#define BEE_CHECK_LOG_LEVEL(level)                                     \
    LogSink sink = nullptr;                                            \
    do {                                                               \
        if (level < detail::gLogLevel.load(std::memory_order_relaxed)) \
            return;                                                    \
        sink = detail::gLogSink.load(std::memory_order_acquire);       \
        if (!sink)                                                     \
            return;                                                    \
    } while (0)

inline void LogRaw(LogLevel level, std::string_view category, std::string_view message, std::source_location loc = std::source_location::current())
{
    BEE_CHECK_LOG_LEVEL(level);

    sink(level, category, message, loc);
}

// --- Log：std::format 风格 ---

template <typename... Args>
void Log(LogLevel level, std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    BEE_CHECK_LOG_LEVEL(level);

    auto message = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    sink(level, category, message, fmt_loc.loc);
}

// --- 便捷函数 ---

template <typename... Args>
void LogTrace(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    BEE_CHECK_LOG_LEVEL(LogLevel::Trace);

    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    sink(LogLevel::Trace, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogDebug(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    BEE_CHECK_LOG_LEVEL(LogLevel::Debug);

    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    sink(LogLevel::Debug, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogInfo(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    BEE_CHECK_LOG_LEVEL(LogLevel::Info);

    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    sink(LogLevel::Info, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogWarn(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    BEE_CHECK_LOG_LEVEL(LogLevel::Warn);

    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    sink(LogLevel::Warn, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogError(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    BEE_CHECK_LOG_LEVEL(LogLevel::Error);

    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    sink(LogLevel::Error, category, msg, fmt_loc.loc);
}

template <typename... Args>
void LogFatal(std::string_view category, FormatWithLocation<std::type_identity_t<Args>...> fmt_loc, Args&&... args)
{
    BEE_CHECK_LOG_LEVEL(LogLevel::Fatal);

    auto msg = std::format(fmt_loc.fmt, std::forward<Args>(args)...);
    sink(LogLevel::Fatal, category, msg, fmt_loc.loc);
}

#undef BEE_CHECK_LOG_LEVEL

} // namespace bee
