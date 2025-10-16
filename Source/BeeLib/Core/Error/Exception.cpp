/**
 * @File Exception.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/12
 * @Brief This file is part of Bee.
 */

#include "Exception.hpp"
#include <stacktrace>

using namespace Bee;

BEE_NORETURN void Bee::ThrowException(const std::source_location& loc, std::string_view msg)
{
    std::string message;
    message.reserve(256);

    std::format_to(std::back_inserter(message), "Error: {}\nAt {}:{}:{}", msg, loc.file_name(), loc.line(), loc.column());

    if (auto stackTrace = std::stacktrace::current(); !stackTrace.empty())
    {
        std::format_to(std::back_inserter(message), "\nStack trace:\n{}", stackTrace);
    }

    BEE_DEBUG_BREAK();
    throw Bee::RuntimeError(message);
}

BEE_NORETURN void Bee::ReportAssertion(const std::source_location& loc, std::string_view cond, std::string_view msg)
{
    std::string message;
    message.reserve(256);

    if (msg.empty())
    {
        std::format_to(std::back_inserter(message), "Assertion: Condition failed: '{}'", cond);
    }
    else
    {
        std::format_to(std::back_inserter(message), "Assertion: Condition failed: '{}'\nMessage: {}", cond, msg);
    }

    std::format_to(std::back_inserter(message), "\nAt: {}:{} ({})", loc.file_name(), loc.line(), loc.function_name());
    if (auto stackTrace = std::stacktrace::current(); !stackTrace.empty())
    {
        std::format_to(std::back_inserter(message), "\nStack trace:\n{}", stackTrace);
    }

    BEE_DEBUG_BREAK();
    throw Bee::AssertionError(message);
}
