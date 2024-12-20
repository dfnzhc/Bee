/**
 * @File Logger.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include <functional>
#include <string>
#include <fmt/format.h>
#include <source_location>
#include <mutex>

#include "Core/Defines.hpp"

namespace bee {
using LogNotifyType = std::function<void(const std::string&)>;

class BEE_API Logger
{
public:
    /// 日志等级
    enum class Level
    {
        Fatal,
        Error,
        Warning,
        Info,
    };

    static Logger& Instance()
    {
        static Logger logger;
        return logger;
    }

    void log(Level level, std::string_view msg);

    void setLevel(Level level);
    Level level();

    void subscribe(std::string_view name, LogNotifyType&& notify);
    void unsubscribe(std::string_view name);

private:
    Logger()  = default;
    ~Logger() = default;

    std::mutex _mutex    = {};
    Logger::Level _level = Logger::Level::Info;
    
    std::unordered_map<std::string_view, LogNotifyType> _subscribers = {};
};

inline auto operator<=>(Logger::Level lhs, Logger::Level rhs)
{
    return static_cast<std::underlying_type_t<Logger::Level>>(lhs) <=> static_cast<std::underlying_type_t<Logger::Level>>(rhs);
}

// @formatter:off

inline void LogInfo(std::string_view msg)
{
    Logger::Instance().log(Logger::Level::Info, msg);
}

template<typename... Args> inline void LogInfo(fmt::format_string<Args...> format, Args&&... args)
{
    Logger::Instance().log(Logger::Level::Info, fmt::format(format, std::forward<Args>(args)...));
}

inline void LogWarn(std::string_view msg)
{
    Logger::Instance().log(Logger::Level::Warning, msg);
}

template<typename... Args> inline void LogWarn(fmt::format_string<Args...> format, Args&&... args)
{
    Logger::Instance().log(Logger::Level::Warning, fmt::format(format, std::forward<Args>(args)...));
}

namespace detail {

void BEE_API LogWithSourceLocation(Logger::Level level, std::source_location sl, std::string_view msg);

template<typename... Args>
inline void LogWithSourceLocation(Logger::Level level, std::source_location sl, fmt::format_string<Args...> fmt, Args&&... args)
{
    LogWithSourceLocation(level, sl, fmt::format(fmt, std::forward<Args>(args)...));
}

} // namespace detail

#define LogError(...) detail::LogWithSourceLocation(Logger::Level::Error, std::source_location::current(), __VA_ARGS__)

#define LogFatal(...) detail::LogWithSourceLocation(Logger::Level::Fatal, std::source_location::current(), __VA_ARGS__)

// @formatter:on
} // namespace bee
