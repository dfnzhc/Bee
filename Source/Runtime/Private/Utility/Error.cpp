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

void bee::AssertHandler(const libassert::assertion_info& assertion)
{
    throw bee::AssertionError(assertion.to_string());
}