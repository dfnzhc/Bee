/**
 * @File Error.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/6/22
 * @Brief This file is part of Bee.
 */

#include "Base/Diagnostics/Error.hpp"

#include <cstdio>
#include <ctime>
#include <format>

namespace bee
{

auto Error::format() const -> std::string
{
    std::string result = std::format("[severity={}] {}", enum_to_name(severity), message);

    if (errc != 0) {
        result += std::format("\nerrc={}", errc);
    }

    result += std::format("\nwhere={}:{} in {}", where.file_name(), where.line(), where.function_name());

#ifndef NDEBUG
    {
        auto    t = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm{};
    #if defined(_WIN32)
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif
        char time_buf[32];
        std::strftime(time_buf, sizeof(time_buf), "%F %T", &tm);
        result += std::format("\ntimestamp={}", time_buf);
    }

    if (!context.empty()) {
        result += "\ncontext:";
        for (const auto& [k, v] : context) {
            result += std::format("\n  - {} = {}", k, v);
        }
    }

    #if defined(__cpp_lib_stacktrace)
    if (trace.has_value()) {
        result += std::format("\nstacktrace:\n{}", std::to_string(trace.value()));
    }
    #endif
#endif

    return result;
}

[[noreturn]] void panic(Error e)
{
    auto details = std::format("severity={}, errc={}", enum_to_name(e.severity), e.errc);
    detail::check_fail("Panic", e.message, details, e.where);
    std::abort(); // unreachable, but satisfies [[noreturn]] for all compilers
}

} // namespace bee
