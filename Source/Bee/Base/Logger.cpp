/**
 * @File Logger.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/27
 * @Brief This file is part of Bee.
 */

#include "Base/Logger.hpp"
#include "Config.hpp"

#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>

using namespace bee;

namespace {
quill::Logger* gConsoleLogger = nullptr;
quill::Logger* gFileLogger    = nullptr;
} // namespace 

void bee::Logger::Setup(bool buildFileLogger)
{
    quill::Backend::start();

    {
        auto sink         = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
        auto formatterOps = quill::PatternFormatterOptions{"[%(time)][%(thread_id)]: %(message)",
                                                           "%Y.%m.%d-%H:%M:%S.%Qms",
                                                           quill::Timezone::LocalTime};

        gConsoleLogger = quill::Frontend::create_or_get_logger("Console", std::move(sink), formatterOps);
        gConsoleLogger->set_log_level(quill::LogLevel::TraceL3);
    }

    if (buildFileLogger)
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

        gFileLogger = quill::Frontend::create_or_get_logger("File", std::move(sink), formatterOps);
        gFileLogger->set_log_level(quill::LogLevel::TraceL3);
    }
}

quill::Logger* bee::Logger::Get(Type type)
{
    if (type == Type::Console)
        return gConsoleLogger;
    
    if (type == Type::File)
        return gFileLogger;
    
    return nullptr;
}