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
std::atomic<LogSink>  gLogSink{nullptr};
std::atomic<LogLevel> gLogLevel{LogLevel::Info};
} // namespace bee::detail

namespace bee
{

void set_log_sink(LogSink sink) noexcept
{
    detail::gLogSink.store(sink, std::memory_order_release);
}

void set_log_level(LogLevel level) noexcept
{
    detail::gLogLevel.store(level, std::memory_order_release);
}

LogSink get_log_sink() noexcept
{
    return detail::gLogSink.load(std::memory_order_acquire);
}

LogLevel get_log_level() noexcept
{
    return detail::gLogLevel.load(std::memory_order_acquire);
}

void enable_default_logging(LogLevel level) noexcept
{
    set_log_sink(default_console_sink);
    set_log_level(level);
}

void default_console_sink(LogLevel level, std::string_view category, std::string_view message, std::source_location location)
{
    std::string_view file = location.file_name();
    if (auto pos = file.find_last_of("/\\"); pos != std::string_view::npos) {
        file = file.substr(pos + 1);
    }

    std::fprintf(
            stderr,
            "[%-5.*s][%.*s] %.*s (%.*s:%u)\n",
            static_cast<int>(enum_to_name(level).size()),
            enum_to_name(level).data(),
            static_cast<int>(category.size()),
            category.data(),
            static_cast<int>(message.size()),
            message.data(),
            static_cast<int>(file.size()),
            file.data(),
            location.line()
    );
}

} // namespace bee
