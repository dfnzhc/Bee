/**
 * @File Error.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/31
 * @Brief This file is part of Bee.
 */

#include "Core/Error.hpp"
#include <stacktrace>

using namespace bee;

void bee::ThrowException(const std::source_location& loc, StringView msg)
{
    std::string fullMsg  = std::format("{}\n\n{}:{} ({})", msg.data(), loc.file_name(), loc.line(), loc.function_name());
    fullMsg             += std::format("\n\nStacktrace:\n{}", std::stacktrace::current(1));

    throw bee::RuntimeError(fullMsg);
}

void bee::ReportAssertion(const std::source_location& loc, StringView cond, StringView msg)
{
    // clang-format off
    std::string fullMsg = std::format("'{}'\n{}\n{}:{} ({})",
                                      cond, msg,
                                      loc.file_name(), loc.line(), loc.function_name());
    // clang-format on
    fullMsg += std::format("\n\nStacktrace:\n{}", std::stacktrace::current(1));

    throw bee::AssertionError(fullMsg);
}