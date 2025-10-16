/**
 * @File ILogSink.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include <memory>
#include "./LogMessage.hpp"

namespace Bee
{
    class ILogSink
    {
    public:
        virtual ~ILogSink() = default;

        virtual void write(const LogMessage& message) = 0;
        virtual void flush() = 0;
        
        virtual bool isValid() const = 0;
    };

    using LogSinkPtr = std::unique_ptr<ILogSink>;
} // namespace Bee
