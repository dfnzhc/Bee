/**
 * @File Check.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/6/22
 * @Brief This file is part of Bee.
 */

#include "Base/Check.hpp"

#include <cstdio>
#include <cstdlib>
#include <format>
#include <string>

namespace bee::detail
{
std::atomic<FailureHandler> g_failure_handler{nullptr};
} // namespace bee::detail

namespace bee
{

void SetFailureHandler(FailureHandler handler) noexcept
{
    detail::g_failure_handler.store(handler, std::memory_order_release);
}

FailureHandler GetFailureHandler() noexcept
{
    return detail::g_failure_handler.load(std::memory_order_acquire);
}

namespace detail
{

    [[noreturn]] void check_fail(std::string_view check_type, std::string_view expr, std::string_view message, std::source_location loc)
    {
        // 1. Format context string
        std::string context;
        if (message.empty()) {
            context = std::format("{} failed: {}", check_type, expr);
        } else {
            context = std::format("{} failed: {} -- {}", check_type, expr, message);
        }

        // 2. Log via the Log system (Fatal level)
        LogRaw(LogLevel::Fatal, "Check", context, loc);

        // 3. Stderr fallback — guarantees output even if no Log sink is installed
        if (!GetLogSink()) {
            std::string_view file = loc.file_name();
            if (auto pos = file.find_last_of("/\\"); pos != std::string_view::npos)
                file = file.substr(pos + 1);

            std::fprintf(stderr, "[Fatal][Check] %.*s (%.*s:%u)\n",
                         static_cast<int>(context.size()), context.data(),
                         static_cast<int>(file.size()), file.data(),
                         loc.line());
        }

        // 4. Call failure handler (if installed)
        auto handler = g_failure_handler.load(std::memory_order_acquire);
        if (handler) {
            handler(expr, message, loc);
        }

        // 5. Debug break (debug builds only)
        #ifndef NDEBUG
        BEE_DEBUG_BREAK;
        #endif

        // 6. Terminate
        std::abort();
    }

} // namespace detail
} // namespace bee
