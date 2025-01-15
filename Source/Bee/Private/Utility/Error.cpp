/**
 * @File Error.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/21
 * @Brief This file is part of Bee.
 */

#include "Utility/Error.hpp"
#include <stacktrace>

using namespace bee;

void bee::ThrowException(const std::source_location& loc, StringView msg)
{
    std::string fullMsg  = std::format("{}\n\n{}:{} ({})", msg.data(), loc.file_name(), loc.line(), loc.function_name());
    fullMsg             += std::format("\n\nStacktrace:\n{}", std::stacktrace::current(1));

    throw bee::RuntimeError(fullMsg);
}

void bee::ReportAssertion(const std::source_location& loc, std::string_view cond, std::string_view msg)
{
    // clang-format off
    std::string fullMsg = std::format("'{}'\n{}{}\n{}:{} ({})",
                                      cond, msg, msg.empty() ? "" : "\n",
                                      loc.file_name(), loc.line(), loc.function_name());
    // clang-format on
    fullMsg += std::format("\n\nStacktrace:\n{}", std::stacktrace::current(1));

    throw bee::AssertionError(fullMsg);
}
