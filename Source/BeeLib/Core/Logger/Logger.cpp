/**
 * @File Logger.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/13
 * @Brief This file is part of Bee.
 */

#include "./Logger.hpp"
#include "BaseSink.hpp"

using namespace Bee;

Logger& Logger::Instance()
{
    static Logger instance;
    if (!instance.isInitialized())
    {
        instance.initialize();
    }
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

void Logger::initialize()
{
    // Initialize 不会在多线程条件下调用

    if (_initialized.load())
        return;

    try
    {
        std::unique_lock lock(_sinksMutex);
        _sinks.clear();
        auto sink = std::make_unique<BaseSink>();
        if (!sink->isValid())
        {
            return;
        }
        _sinks.emplace_back(std::move(sink));
    }
    catch (...)
    {
        return;
    }

    _initialized.store(true);
}

bool Logger::isInitialized() const
{
    return _initialized.load();
}

void Logger::shutdown()
{
    flush();
    clearSinks();

    _initialized.store(false);
}

Logger::~Logger()
{
    if (_initialized.load())
    {
        shutdown();
    }
}

bool Logger::shouldLog(LogMessage::Level level) const
{
    return _globalMinLevel <= level;
}
