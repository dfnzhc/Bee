/**
 * @File Log.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/25
 * @Brief Host 和 GPU 上的日志工具
 */

#pragma once

#include <format>
#include <shared_mutex>
#include <source_location>

#include "Bee/Core/Defines.hpp"
#include "Bee/Core/Macros.hpp"

#ifdef BEE_HOST_CODE
#include <iostream>
#include <string_view>
#include <vector>
#endif

namespace bee
{
struct LogMessage
{
    enum class Level : u8
    {
        Trace = 0,
        Info  = 1,
        Warn  = 2,
        Error = 3,
        Fatal = 4
    };

    std::source_location location;
    std::string message;
    Level level;

    LogMessage(Level l, const std::string_view msg, std::source_location loc = std::source_location::current()) :
        location(loc), message(msg), level(l)
    {
    }
};

class ILogSink
{
public:
    virtual ~ILogSink() = default;

    virtual void write(const LogMessage& message) = 0;
    virtual void flush() = 0;

    virtual bool isValid() const = 0;
};

using LogSinkPtr = std::unique_ptr<ILogSink>;

// Host 端的日志框架，具体的 LogSink 需要自己添加
class Logger
{
public:
    static Logger& Instance();

    void log(LogMessage::Level level, std::string_view tag, std::string_view message,
             std::source_location location = std::source_location::current()) const;

    template <typename... Args>
    void logFormat(LogMessage::Level level, std::source_location location, std::string_view tag, std::format_string<Args...> fmt, Args&&... args)
    {
        if (!shouldLog(level))
            return;

        thread_local std::string buffer;
        buffer.clear();
        std::format_to(std::back_inserter(buffer), fmt, std::forward<Args>(args)...);
        log(level, tag, buffer, location);
    }

    void addSink(LogSinkPtr sink);
    void removeSink(ILogSink* sink);
    void clearSinks();

    void setMinLevel(LogMessage::Level level);
    void flush() const;

private:
    Logger() = default;
    ~Logger();

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    bool shouldLog(LogMessage::Level level) const;

private:
    std::vector<LogSinkPtr> _sinks;
    mutable std::shared_mutex _sinksMutex;

    LogMessage::Level _globalMinLevel{LogMessage::Level::Trace};
};

template <typename... Args>
void LogFormat(LogMessage::Level level, std::source_location loc, std::string_view tag, std::format_string<Args...> fmt, Args&&... args)
{
    Logger::Instance().logFormat(level, loc, tag, fmt, std::forward<Args>(args)...);
}

// clang-format off
#ifdef BEE_GPU_CODE
template <typename... Args>
BEE_FUNC void LogDevice(LogMessage::Level level, const std::source_location& loc, const char* msg, Args&&... /*args*/)
{
    printf ("[Device] [%d] [%s:%d]: %s\n", static_cast<int>(level), loc.file_name(), loc.line(), msg);
}
#endif // BEE_GPU_CODE
// clang-format on

} // namespace bee


#ifdef BEE_GPU_CODE

#define BEE_TRACE(msg, ...) do { bee::LogDevice(::bee::LogMessage::Level::Trace, std::source_location::current(), msg, __VA_ARGS__); } while(0)
#define BEE_INFO(msg, ...)  do { bee::LogDevice(::bee::LogMessage::Level::Info, std::source_location::current(), msg, __VA_ARGS__); } while(0)
#define BEE_WARN(msg, ...)  do { bee::LogDevice(::bee::LogMessage::Level::Warn, std::source_location::current(), msg, __VA_ARGS__); } while(0)
#define BEE_ERROR(msg, ...) do { bee::LogDevice(::bee::LogMessage::Level::Error, std::source_location::current(), msg, __VA_ARGS__); } while(0)
#define BEE_FATAL(msg, ...) do { bee::LogDevice(::bee::LogMessage::Level::Fatal, std::source_location::current(), msg, __VA_ARGS__); } while(0)

#else

#ifndef BEE_LOG_TAG
#define BEE_LOG_TAG "🐝"
#endif

#define BEE_TRACE(msg, ...) do { ::bee::LogFormat(::bee::LogMessage::Level::Trace, std::source_location::current(), BEE_LOG_TAG, msg __VA_OPT__(,) ##__VA_ARGS__); } while(0)
#define BEE_INFO(msg, ...)  do { ::bee::LogFormat(::bee::LogMessage::Level::Info,  std::source_location::current(), BEE_LOG_TAG, msg __VA_OPT__(,) ##__VA_ARGS__); } while(0)
#define BEE_WARN(msg, ...)  do { ::bee::LogFormat(::bee::LogMessage::Level::Warn,  std::source_location::current(), BEE_LOG_TAG, msg __VA_OPT__(,) ##__VA_ARGS__); } while(0)
#define BEE_ERROR(msg, ...) do { ::bee::LogFormat(::bee::LogMessage::Level::Error, std::source_location::current(), BEE_LOG_TAG, msg __VA_OPT__(,) ##__VA_ARGS__); } while(0)
#define BEE_FATAL(msg, ...) do { ::bee::LogFormat(::bee::LogMessage::Level::Fatal, std::source_location::current(), BEE_LOG_TAG, msg __VA_OPT__(,) ##__VA_ARGS__); } while(0)

#endif // BEE_GPU_CODE
