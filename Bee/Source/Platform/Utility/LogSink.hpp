/**
 * @File LogSink.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Bee/Core/Log.hpp"

namespace spdlog
{
class logger;

namespace details { class thread_pool; }

} // namespace spdlog

namespace bee
{

class LogSink : public ILogSink
{
public:
    LogSink();
    ~LogSink() override;

    void write(const LogMessage& message) override;
    void flush() override;

    bool isValid() const override;

private:
    void setup();
    void shutdown();

private:
    std::shared_ptr<spdlog::details::thread_pool> ThreadPool = nullptr;
    std::shared_ptr<spdlog::logger> _logger = nullptr;
    
    bool _started                           = false;
};

} // namespace bee
