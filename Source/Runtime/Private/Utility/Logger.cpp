/**
 * @File Logger.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#include "Utility/Logger.hpp"
#include "Utility/Error.hpp"

#include <fmt/format.h>
#include <fmt/color.h>
#include <iostream>
#include <ranges>

using namespace bee;

namespace {
constexpr inline const char* logLevelString(Logger::Level level)
{
    switch (level) {
    case Logger::Level::Fatal   : return "灾难";
    case Logger::Level::Error   : return "错误";
    case Logger::Level::Warning : return "警告";
    case Logger::Level::Info    : return "信息";
    }
    BEE_UNREACHABLE();
}

} // namespace

void Logger::log(Level level, std::string_view msg)
{
    auto lock = std::lock_guard(_mutex);

    if (level > _level)
        return;

    fmt::color color = fmt::color::white;
    switch (level) {
    case Logger::Level::Fatal   : color = fmt::color::red; break;
    case Logger::Level::Error   : color = fmt::color::magenta; break;
    case Logger::Level::Warning : color = fmt::color::coral; break;
    case Logger::Level::Info    : color = fmt::color::green_yellow; break;
    }

    const auto s = fmt::format(fmt::fg(color), "[{}]: {}", logLevelString(level), msg);
    auto& os     = std::cout;//level > Logger::Level::Error ? std::cout : std::cerr;
    os << s << std::endl;
    //    std::flush(os);

    if (!_subscribers.empty()) {
        for (const auto& notify : _subscribers | std::views::values) {
            notify(s);
        }
    }
}

void Logger::setLevel(Level level)
{
    auto lock = std::lock_guard(_mutex);
    _level    = level;
}

Logger::Level Logger::level()
{
    auto lock = std::lock_guard(_mutex);
    return _level;
}

void Logger::subscribe(std::string_view name, LogNotifyType&& notify)
{
    auto lock = std::lock_guard(_mutex);
    _subscribers.emplace(name, std::move(notify));
}

void Logger::unsubscribe(std::string_view name)
{
    auto lock = std::lock_guard(_mutex);
    _subscribers.extract(name);
}

void BEE_API detail::LogWithSourceLocation(Logger::Level level, std::source_location sl, std::string_view msg)
{
    Logger::Instance().log(level, fmt::format("{}: '{}' {}({}:{})", msg, sl.function_name(), sl.file_name(), sl.line(), sl.column()));
}
