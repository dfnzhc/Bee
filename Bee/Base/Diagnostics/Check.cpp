/**
 * @File Check.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/6/22
 * @Brief This file is part of Bee.
 */

#include "Base/Diagnostics/Check.hpp"

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
        // 1. 组装上下文字符串
        std::string context;
        try {
            if (message.empty()) {
                context = std::format("{} failed: {}", check_type, expr);
            } else {
                context = std::format("{} failed: {} -- {}", check_type, expr, message);
            }
        } catch (...) {
            context = "failure formatting failed";
            if (!check_type.empty()) {
                context += ": ";
                context.append(check_type.data(), check_type.size());
            }
            if (!expr.empty()) {
                context += " expr=";
                context.append(expr.data(), expr.size());
            }
        }

        // 2. 通过日志系统记录（Fatal 级别）
        LogRaw(LogLevel::Fatal, "Check", context, loc);

        // 3. stderr 回退输出：即使未安装日志 sink 也保证可见
        if (!get_log_sink()) {
            std::string_view file = loc.file_name();
            if (auto pos = file.find_last_of("/\\"); pos != std::string_view::npos)
                file = file.substr(pos + 1);

            std::fprintf(
                    stderr,
                    "[Fatal][Check] %.*s (%.*s:%u)\n",
                    static_cast<int>(context.size()),
                    context.data(),
                    static_cast<int>(file.size()),
                    file.data(),
                    loc.line()
            );
        }

        // 4. 调用失败处理器（若已安装）
        auto handler = g_failure_handler.load(std::memory_order_acquire);
        if (handler) {
            handler(expr, message, loc);
        }

        // 5. 触发调试中断（仅调试构建）
#ifndef NDEBUG
        BEE_DEBUG_BREAK;
#endif

        // 6. 终止进程
        std::abort();
    }

} // namespace detail
} // namespace bee
