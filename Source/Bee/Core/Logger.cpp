/**
 * @File Logger.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/27
 * @Brief This file is part of Bee.
 */

#include "Core/Logger.hpp"

#include "Config.hpp"
#include "Error.hpp"

#define QUILL_DISABLE_NON_PREFIXED_MACROS
#include <quill/core/Attributes.h>
#include <quill/Logger.h>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>

#include <quill/std/Array.h>
#include <quill/std/Chrono.h>
#include <quill/std/Deque.h>
#include <quill/std/FilesystemPath.h>
#include <quill/std/ForwardList.h>
#include <quill/std/List.h>
#include <quill/std/Map.h>
#include <quill/std/Optional.h>
#include <quill/std/Pair.h>
#include <quill/std/Set.h>
#include <quill/std/Tuple.h>
#include <quill/std/UnorderedMap.h>
#include <quill/std/UnorderedSet.h>
#include <quill/std/Vector.h>
#include <quill/std/WideString.h>

#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>

#include <eventpp/eventdispatcher.h>

using namespace bee;

namespace {
constexpr inline const char* logLevelString(Logger::Level level)
{
    switch (level) {
    case Logger::Level::Fatal: return "Fatal";
    case Logger::Level::Error: return "Error";
    case Logger::Level::Warning: return " Warn";
    case Logger::Level::Info: return " Info";
    case Logger::Level::Debug: return "Debug";
    case Logger::Level::Trace: return "Trace";
    default:;
    }
    BEE_UNREACHABLE();
}

std::vector<quill::Logger*> kLoggers;

// clang-format off
#define LOG_TRACE(fmt, ...) { for (auto* logger : kLoggers) { QUILL_LOG_TRACE_L3(logger, fmt, ## __VA_ARGS__); } }
#define LOG_DEBUG(fmt, ...) { for (auto* logger : kLoggers) { QUILL_LOG_DEBUG(logger, fmt, ## __VA_ARGS__);    } }
#define LOG_INFO(fmt, ...)  { for (auto* logger : kLoggers) { QUILL_LOG_INFO(logger, fmt, ## __VA_ARGS__);     } }
#define LOG_WARN(fmt, ...)  { for (auto* logger : kLoggers) { QUILL_LOG_WARNING(logger, fmt, ## __VA_ARGS__);  } }
#define LOG_ERROR(fmt, ...) { for (auto* logger : kLoggers) { QUILL_LOG_ERROR(logger, fmt, ## __VA_ARGS__);    } }
#define LOG_FATAL(fmt, ...) { for (auto* logger : kLoggers) { QUILL_LOG_CRITICAL(logger, fmt, ## __VA_ARGS__); } }
// clang-format on

struct LogDispatcher
{
    using DispatcherType = eventpp::EventDispatcher<int, void(Logger::Level, const std::string&)>;
    using DispatcherHandle = DispatcherType::Handle;
    using SubscriberRecordMap = std::unordered_map<String, DispatcherHandle>;
    
    SubscriberRecordMap records;
    DispatcherType dispatcher;

    void subscribe(std::string_view name, std::function<void(Logger::Level, const std::string&)>&& notify)
    {
        if (records.contains(name))
            return;
        dispatcher.appendListener(0, std::move(notify));
    }
    
    void unsubscribe(std::string_view name)
    {
        if (!records.contains(name))
            return;

        const auto handle = records.extract(name).mapped();
        dispatcher.removeListener(0, handle);
    }

    void log(Logger::Level level, std::string_view msg) const
    {
        dispatcher.dispatch(0, level, msg.data());
    }
};

std::unique_ptr<LogDispatcher> pLogDispatcher = nullptr;

} // namespace

Logger::Logger()
{
    quill::Backend::start();

    // Default setup two loggers: console and file
    {
        auto sink         = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
        auto formatterOps = quill::PatternFormatterOptions{"[%(time)][%(thread_id)]: %(message)",
                                                           "%Y.%m.%d-%H:%M:%S.%Qms",
                                                           quill::Timezone::LocalTime};

        auto logger = quill::Frontend::create_or_get_logger("Console", std::move(sink), formatterOps);
        logger->set_log_level(quill::LogLevel::TraceL3);
        kLoggers.emplace_back(logger);
    }

    {
        const auto logFilePath = std::string(BEE_DATA_DIR) + "/Bee.log";

        auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
            logFilePath,
            []() {
                quill::FileSinkConfig cfg;
                cfg.set_open_mode('w');
                // cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
                return cfg;
            }(),
            quill::FileEventNotifier{});
        auto formatterOps = quill::PatternFormatterOptions{"[%(time)][%(thread_id)][%(short_source_location)][%(log_level)]%(tags): %(message)",
                                                           "%Y.%m.%d-%H:%M:%S.%Qms", quill::Timezone::LocalTime};

        auto logger = quill::Frontend::create_or_get_logger("File", std::move(sink), formatterOps);
        logger->set_log_level(quill::LogLevel::TraceL3);
        kLoggers.emplace_back(logger);
    }

#ifdef NDEBUG
    setLevel(Level::Info);
#endif

    pLogDispatcher = std::make_unique<LogDispatcher>();
}

Logger::~Logger()
{
    quill::Backend::stop();
}

void Logger::log(Level level, std::string_view msg)
{
    // clang-format off
    switch (level) {
    case Level::Fatal:   LOG_FATAL("{}", msg); break;
    case Level::Error:   LOG_ERROR("{}", msg); break;
    case Level::Warning: LOG_WARN("{}", msg); break;
    case Level::Info:    LOG_INFO("{}", msg); break;
    case Level::Debug:   LOG_DEBUG("{}", msg); break;
    case Level::Trace:   LOG_TRACE("{}", msg); break;
    default: ;
    }
    // clang-format on

    pLogDispatcher->log(level, msg);
}

void Logger::setLevel(Level level)
{
    quill::LogLevel quillLevel = quill::LogLevel::TraceL3;
    // clang-format off
    switch (level) {
    case Level::Fatal:   quillLevel = quill::LogLevel::Critical; break;
    case Level::Error:   quillLevel = quill::LogLevel::Error; break;
    case Level::Warning: quillLevel = quill::LogLevel::Warning; break;
    case Level::Info:    quillLevel = quill::LogLevel::Info; break;
    case Level::Debug:   quillLevel = quill::LogLevel::Debug; break;
    case Level::Trace:   quillLevel = quill::LogLevel::TraceL3; break;
    default: ;
    }
    // clang-format on

    for (auto* logger : kLoggers) {
        logger->set_log_level(quillLevel);
    }
}

void Logger::subscribe(std::string_view name, LogNotifyType&& notify)
{
    auto lock = std::lock_guard(_mutex);
    pLogDispatcher->subscribe(name, std::forward<LogNotifyType>(notify));
}

void Logger::unsubscribe(std::string_view name)
{
    auto lock = std::lock_guard(_mutex);
    pLogDispatcher->unsubscribe(name);
}

void BEE_API detail::LogWithSourceLocation(Logger::Level level, std::source_location sl, std::string_view msg)
{
    Logger::Instance().log(level, std::format("{}: '{}' {}({}:{})", msg, sl.function_name(), sl.file_name(), sl.line(), sl.column()));
}