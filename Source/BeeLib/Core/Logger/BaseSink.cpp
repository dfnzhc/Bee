/**
 * @File BaseSink.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/14
 * @Brief This file is part of Bee.
 */

#include "BaseSink.hpp"

BEE_PUSH_WARNING
BEE_CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
BEE_CLANG_DISABLE_WARNING("-Wswitch-enum")
BEE_CLANG_DISABLE_WARNING("-Wreserved-macro-identifier")
BEE_CLANG_DISABLE_WARNING("-Wduplicate-enum")
BEE_CLANG_DISABLE_WARNING("-Wmissing-noreturn")
BEE_CLANG_DISABLE_WARNING("-Wnonportable-system-include-path")
BEE_CLANG_DISABLE_WARNING("-Wfloat-equal")
BEE_CLANG_DISABLE_WARNING("-Wimplicit-int-conversion")
BEE_CLANG_DISABLE_WARNING("-Wlanguage-extension-token")
BEE_CLANG_DISABLE_WARNING("-Wcovered-switch-default")
BEE_CLANG_DISABLE_WARNING("-Wundefined-func-template")
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
BEE_POP_WARNING

using namespace Bee;

class BaseSink::Impl
{
public:
    void startup()
    {
        #ifdef _WIN32
        // 设置控制台代码页为 UTF-8
        SetConsoleOutputCP(CP_UTF8);
        #endif

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // TODO: 日志路径
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Bee.log", true);

        BEE_PUSH_WARNING
        BEE_CLANG_DISABLE_WARNING("-Wundefined-func-template")
        // TODO: 将线程 id 改为可读的线程名称
        // TODO: Tag-xxx
        // 控制台格式: [级别] [时间] [线程ID] [Logger名]: 消息
        console_sink->set_pattern("[%^%5l%$] [%H:%M:%S.%e] [%t] %v");

        // 文件格式: [日期 时间.毫秒] [级别] [Logger名] [线程ID] 消息
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%8l] [thread %t] %v");
        BEE_POP_WARNING

        // 默认开启所有级别的日志输出
        console_sink->set_level(spdlog::level::trace);
        file_sink->set_level(spdlog::level::trace);
        spdlog::set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

        // clang-format off
        ThreadPool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
        Logger = std::make_shared<spdlog::async_logger>("Bee", sinks.begin(), sinks.end(), ThreadPool, spdlog::async_overflow_policy::block);
        // clang-format on

        Logger->set_level(spdlog::level::trace);
        spdlog::register_logger(Logger);

        Started = true;
    }

    void shutdown()
    {
        if (Started)
        {
            try
            {
                ThreadPool.reset();
                Logger.reset();
                
                spdlog::shutdown();
            }
            catch (std::exception& e)
            {
                BEE_ERROR("[spdlog]: {}", e.what());
            }

            Started = false;
        }
    }

    void write(const LogMessage& logMsg) const
    {
        // clang-format off
        switch (logMsg.level)
        {
            case LogMessage::Level::Trace: SPDLOG_LOGGER_TRACE(Logger.get(), "{}", logMsg.message); break;
            case LogMessage::Level::Info:  SPDLOG_LOGGER_INFO(Logger.get(), "{}", logMsg.message); break;
            case LogMessage::Level::Warn:  SPDLOG_LOGGER_WARN(Logger.get(), "{} ('{}' {}:{})", logMsg.message, logMsg.location.function_name(), logMsg.location.file_name(), logMsg.location.line()); break;
            case LogMessage::Level::Error: SPDLOG_LOGGER_ERROR(Logger.get(), "{} ('{}' {}:{})", logMsg.message, logMsg.location.function_name(), logMsg.location.file_name(), logMsg.location.line()); break;
            case LogMessage::Level::Fatal: SPDLOG_LOGGER_CRITICAL(Logger.get(), "{} ('{}' {}:{})", logMsg.message, logMsg.location.function_name(), logMsg.location.file_name(), logMsg.location.line()); break;
        }
        // clang-format on
    }

    void flush() const
    {
        try
        {
            Logger->flush();
        }
        catch (std::exception& e)
        {
            BEE_ERROR("[spdlog]: {}", e.what());
        }
    }

    bool isValid() const
    {
        return Logger && Started;
    }

    std::shared_ptr<spdlog::details::thread_pool> ThreadPool = nullptr;
    std::shared_ptr<spdlog::logger> Logger                   = nullptr;

    bool Started = false;
};

BaseSink::BaseSink() :
    _impl(std::make_unique<Impl>())
{
    if (_impl)
        _impl->startup();
}

BaseSink::~BaseSink()
{
    if (_impl)
        _impl->shutdown();
}

void BaseSink::write(const LogMessage& message)
{
    if (_impl)
        _impl->write(message);
}

void BaseSink::flush()
{
    if (_impl)
        _impl->flush();
}

bool BaseSink::isValid() const
{
    return _impl != nullptr && _impl->isValid();
}
