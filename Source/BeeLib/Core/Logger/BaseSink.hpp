/**
 * @File BaseSink.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include "./ILogSink.hpp"

namespace Bee
{
    class BaseSink : public ILogSink
    {
    public:
        BaseSink();
        ~BaseSink() override;

        void write(const LogMessage& message) override;
        void flush() override;
        
        bool isValid() const override;
    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };
} // namespace Bee
