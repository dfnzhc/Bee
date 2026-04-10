/**
 * @File Log.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/9
 * @Brief This file is part of Bee.
 */

#include "Base/Diagnostics/Log.hpp"

#include <cstdio>

namespace bee::detail
{
std::atomic<LogSink> g_log_sink{nullptr};
std::atomic<LogLevel> g_log_level{LogLevel::Info};
} // namespace bee::detail

namespace bee
{

void SetLogSink(LogSink sink) noexcept
{
    detail::g_log_sink.store(sink, std::memory_order_release);
}

void SetLogLevel(LogLevel level) noexcept
{
    detail::g_log_level.store(level, std::memory_order_release);
}

LogSink GetLogSink() noexcept
{
    return detail::g_log_sink.load(std::memory_order_acquire);
}

LogLevel GetLogLevel() noexcept
{
    return detail::g_log_level.load(std::memory_order_acquire);
}

void EnableDefaultLogging(LogLevel level) noexcept
{
    SetLogSink(DefaultConsoleSink);
    SetLogLevel(level);
}

void DefaultConsoleSink(LogLevel level, std::string_view category, std::string_view message, std::source_location location)
{
    std::string_view file = location.file_name();
    if (auto pos = file.find_last_of("/\\"); pos != std::string_view::npos) {
        file = file.substr(pos + 1);
    }

    std::fprintf(stderr, "[%-5.*s][%.*s] %.*s (%.*s:%u)\n",
                 static_cast<int>(LogLevelToString(level).size()), LogLevelToString(level).data(),
                 static_cast<int>(category.size()), category.data(),
                 static_cast<int>(message.size()), message.data(),
                 static_cast<int>(file.size()), file.data(),
                 location.line());
}

} // namespace bee
