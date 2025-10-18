/**
 * @File Logger.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/12
 * @Brief This file is part of Bee.
 */

#pragma once

#include <shared_mutex>
#include "./LogMessage.hpp"
#include "./ILogSink.hpp"

namespace Bee
{
    class Logger
    {
    public:
        static Logger& Instance();

        void log(LogMessage::Level level, std::string_view tag, std::string_view message,
                 std::source_location location = std::source_location::current()) const;

        template <typename... Args>
        void logFormat(LogMessage::Level level, std::source_location location, std::string_view tag,
                       std::format_string<Args...> fmt, Args&&... args)
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

        void initialize();
        bool isInitialized() const;
        void shutdown();

    private:
        Logger() = default;
        ~Logger();

        Logger(const Logger&)            = delete;
        Logger& operator=(const Logger&) = delete;

        bool shouldLog(LogMessage::Level level) const;

        std::vector<LogSinkPtr> _sinks;
        mutable std::shared_mutex _sinksMutex;

        LogMessage::Level _globalMinLevel{LogMessage::Level::Trace};
        std::atomic<bool> _initialized{false};
    };

    template <typename... Args>
    void LogFormat(LogMessage::Level level, std::source_location loc, std::string_view tag, std::format_string<Args...> fmt, Args&&... args)
    {
        Logger::Instance().logFormat(level, loc, tag, fmt, std::forward<Args>(args)...);
    }
} // namespace Bee

#ifndef BEE_LOG_TAG
#define BEE_LOG_TAG "🐝"
#endif

// clang-format off
#define BEE_TRACE(msg, ...) do { ::Bee::LogFormat(::Bee::LogMessage::Level::Trace, std::source_location::current(), BEE_LOG_TAG, msg, ##__VA_ARGS__); } while(0)
#define BEE_INFO(msg, ...)  do { ::Bee::LogFormat(::Bee::LogMessage::Level::Info,  std::source_location::current(), BEE_LOG_TAG, msg, ##__VA_ARGS__); } while(0)
#define BEE_WARN(msg, ...)  do { ::Bee::LogFormat(::Bee::LogMessage::Level::Warn,  std::source_location::current(), BEE_LOG_TAG, msg, ##__VA_ARGS__); } while(0)
#define BEE_ERROR(msg, ...) do { ::Bee::LogFormat(::Bee::LogMessage::Level::Error, std::source_location::current(), BEE_LOG_TAG, msg, ##__VA_ARGS__); } while(0)
#define BEE_FATAL(msg, ...) do { ::Bee::LogFormat(::Bee::LogMessage::Level::Fatal, std::source_location::current(), BEE_LOG_TAG, msg, ##__VA_ARGS__); } while(0)

// clang-format on
