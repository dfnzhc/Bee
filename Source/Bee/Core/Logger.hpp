/**
 * @File Logger.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

#include <mutex>
#include <format>
#include <functional>
#include <source_location>

namespace bee {

class BEE_API Logger
{
public:
    enum class Level
    {
        Fatal,
        Error,
        Warning,
        Info,
        Debug,
        Trace
    };

    static Logger& Instance()
    {
        static Logger logger;
        return logger;
    }

    void log(Level level, std::string_view msg);

    void setLevel(Level level);

    using LogNotifyType = std::function<void(Level, const std::string&)>;
    void subscribe(std::string_view name, LogNotifyType&& notify);
    void unsubscribe(std::string_view name);

private:
    Logger();
    ~Logger();

    std::mutex _mutex = {};
};

inline auto operator<=>(Logger::Level lhs, Logger::Level rhs)
{
    return static_cast<std::underlying_type_t<Logger::Level>>(lhs) <=> static_cast<std::underlying_type_t<Logger::Level>>(rhs);
}

// @formatter:off

inline void LogDebug(std::string_view msg)
{
    Logger::Instance().log(Logger::Level::Debug, msg);
}

template<typename... Args> inline void LogDebug(std::format_string<Args...> format, Args&&... args)
{
    Logger::Instance().log(Logger::Level::Debug, std::format(format, std::forward<Args>(args)...));
}

inline void LogInfo(std::string_view msg)
{
    Logger::Instance().log(Logger::Level::Info, msg);
}

template<typename... Args> inline void LogInfo(std::format_string<Args...> format, Args&&... args)
{
    Logger::Instance().log(Logger::Level::Info, std::format(format, std::forward<Args>(args)...));
}

inline void LogVerbose(std::string_view msg)
{
#ifdef BEE_ENABLE_DEBUG
    Logger::Instance().log(Logger::Level::Info, msg);
#endif
}

template<typename... Args> inline void LogVerbose(std::format_string<Args...> format, Args&&... args)
{
#ifdef BEE_ENABLE_DEBUG
    Logger::Instance().log(Logger::Level::Info, std::format(format, std::forward<Args>(args)...));
#endif
}

inline void LogWarn(std::string_view msg)
{
    Logger::Instance().log(Logger::Level::Warning, msg);
}

template<typename... Args> inline void LogWarn(std::format_string<Args...> format, Args&&... args)
{
    Logger::Instance().log(Logger::Level::Warning, std::format(format, std::forward<Args>(args)...));
}

namespace detail {

void BEE_API LogWithSourceLocation(Logger::Level level, std::source_location sl, std::string_view msg);

template<typename... Args>
inline void LogWithSourceLocation(Logger::Level level, std::source_location sl, std::format_string<Args...> fmt, Args&&... args)
{
    LogWithSourceLocation(level, sl, std::format(fmt, std::forward<Args>(args)...));
}

} // namespace detail

#define LogError(...) detail::LogWithSourceLocation(Logger::Level::Error, std::source_location::current(), __VA_ARGS__)

#define LogFatal(...) detail::LogWithSourceLocation(Logger::Level::Fatal, std::source_location::current(), __VA_ARGS__)

// @formatter:on
} // namespace bee