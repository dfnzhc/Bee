/**
 * @File Log.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/25
 * @Brief This file is part of Bee.
 */

#include "Log.hpp"
#include <mutex>

#ifdef BEE_HOST_CODE

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace bee;

Logger& Logger::Instance()
{
    static Logger instance;
    return instance;
}

void Logger::log(LogMessage::Level level, std::string_view tag, std::string_view message, std::source_location location) const
{
    if (!shouldLog(level))
        return;

    LogMessage msg{level, "", location};
    std::format_to(std::back_inserter(msg.message), "|{}|: {}", tag, message.data());

    std::unique_lock lock(_sinksMutex);
    for (const auto& sink : _sinks)
        sink->write(msg);
}

void Logger::addSink(LogSinkPtr sink)
{
    std::unique_lock lock(_sinksMutex);
    _sinks.push_back(std::move(sink));
}

void Logger::removeSink(ILogSink* sink)
{
    std::unique_lock lock(_sinksMutex);
    _sinks.erase(std::ranges::remove_if(_sinks, [=](const LogSinkPtr& sinkPtr)
    {
        return sinkPtr.get() == sink;
    }).begin(), _sinks.end());
}

void Logger::clearSinks()
{
    std::unique_lock lock(_sinksMutex);
    _sinks.clear();
}

void Logger::setMinLevel(LogMessage::Level level)
{
    _globalMinLevel = level;
}

void Logger::flush() const
{
    std::unique_lock lock(_sinksMutex);
    for (const auto& sink : _sinks)
        sink->flush();
}

Logger::~Logger()
{
    flush();
    clearSinks();
}

bool Logger::shouldLog(LogMessage::Level level) const
{
    return _globalMinLevel <= level;
}

#endif // BEE_HOST_CODE
