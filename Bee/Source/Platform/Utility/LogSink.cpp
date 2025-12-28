/**
 * @File LogSink.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/30
 * @Brief This file is part of Bee.
 */

#include "LogSink.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace bee;

LogSink::LogSink()
{
    setup();
}

LogSink::~LogSink()
{
    shutdown();
}

void LogSink::write(const LogMessage& message)
{
    // clang-format off
    switch (message.level)
    {
    case LogMessage::Level::Trace: SPDLOG_LOGGER_TRACE(Logger.get(), "{}", message.message); break;
    case LogMessage::Level::Info:  SPDLOG_LOGGER_INFO(_logger.get(), "{}", message.message); break;
    case LogMessage::Level::Warn:  SPDLOG_LOGGER_WARN(_logger.get(), "{} ('{}' {}:{})", message.message, message.location.function_name(), message.location.file_name(), message.location.line()); break;
    case LogMessage::Level::Error: SPDLOG_LOGGER_ERROR(_logger.get(), "{} ('{}' {}:{})", message.message, message.location.function_name(), message.location.file_name(), message.location.line()); break;
    case LogMessage::Level::Fatal: SPDLOG_LOGGER_CRITICAL(_logger.get(), "{} ('{}' {}:{})", message.message, message.location.function_name(), message.location.file_name(), message.location.line()); break;
    }
    // clang-format on
}

void LogSink::flush()
{
    try
    {
        _logger->flush();
    }
    catch (std::exception& e)
    {
        BEE_ERROR("[spdlog]: {}", e.what());
    }
}

bool LogSink::isValid() const
{
    return _logger && _started;
}

void LogSink::setup()
{
    #ifdef _WIN32
    // 设置控制台代码页为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    #endif

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    // TODO: 日志路径
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Bee.log", true);

    // TODO: 将线程 id 改为可读的线程名称
    // TODO: Tag-xxx
    // 控制台格式: [级别] [时间] [线程ID] [Logger名]: 消息
    console_sink->set_pattern("[%^%5l%$] [%H:%M:%S.%e] [%t] %v");

    // 文件格式: [日期 时间.毫秒] [级别] [Logger名] [线程ID] 消息
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%8l] [thread %t] %v");

    // 默认开启所有级别的日志输出
    console_sink->set_level(spdlog::level::trace);
    file_sink->set_level(spdlog::level::trace);
    spdlog::set_level(spdlog::level::trace);

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

    ThreadPool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
    _logger    = std::make_shared<spdlog::async_logger>("Bee", sinks.begin(), sinks.end(), ThreadPool, spdlog::async_overflow_policy::block);

    _logger->set_level(spdlog::level::trace);
    spdlog::register_logger(_logger);

    _started = true;
}

void LogSink::shutdown()
{
    if (_started)
    {
        try
        {
            ThreadPool.reset();
            _logger.reset();

            spdlog::shutdown();
        }
        catch (std::exception& e)
        {
            BEE_ERROR("[spdlog]: {}", e.what());
        }

        _started = false;
    }
}
