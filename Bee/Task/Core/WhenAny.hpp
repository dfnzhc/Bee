/**
 * @File WhenAny.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "Base/Diagnostics/Check.hpp"
#include "Task/Core/Task.hpp"

namespace bee
{

// =========================================================================
// WhenAnyResult<T>
// =========================================================================

template <typename T>
struct WhenAnyResult
{
    std::size_t index; ///< 最先完成的任务的索引
    T           value; ///< 其结果值
};

// =========================================================================
// detail — when_any 内部实现
// =========================================================================

namespace detail
{

    template <typename T>
    struct WhenAnyControl
    {
        std::atomic<std::size_t> remaining;
        std::coroutine_handle<>  parent{nullptr};
        std::atomic<bool>        claimed{false};

        // 获胜者的结果
        std::optional<WhenAnyResult<T>> winner;
        std::exception_ptr              winner_ex;

        explicit WhenAnyControl(std::size_t n)
            : remaining(n + 1)
        {
        }
    };

    template <typename T>
    auto when_any_wrapper(std::shared_ptr<WhenAnyControl<T>> ctrl, std::size_t index, Task<T> task) -> DetachedTask
    {
        static_assert(!std::is_void_v<T>, "when_any 暂不支持 void 类型");

        std::optional<T>   result_val;
        std::exception_ptr ex;

        try {
            result_val.emplace(co_await std::move(task));
        } catch (...) {
            ex = std::current_exception();
        }

        // 尝试成为获胜者（CAS 选举）
        bool expected = false;
        if (ctrl->claimed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            if (ex) {
                ctrl->winner_ex = ex;
            } else {
                ctrl->winner.emplace(WhenAnyResult<T>{index, std::move(*result_val)});
            }
        }

        // 最后一个完成的 wrapper 恢复 parent（结构化并发：等待所有任务完成）
        if (ctrl->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            ctrl->parent.resume();
        }
    }

    template <typename T>
    struct WhenAnyAwaiter
    {
        std::shared_ptr<WhenAnyControl<T>> ctrl;
        std::vector<Task<T>>&              tasks;

        [[nodiscard]] auto await_ready() const noexcept -> bool
        {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> h) -> bool
        {
            ctrl->parent = h;

            for (std::size_t i = 0; i < tasks.size(); ++i) {
                when_any_wrapper<T>(ctrl, i, std::move(tasks[i]));
            }

            return ctrl->remaining.fetch_sub(1, std::memory_order_acq_rel) != 1;
        }

        auto await_resume() -> WhenAnyResult<T>
        {
            if (ctrl->winner_ex) {
                std::rethrow_exception(ctrl->winner_ex);
            }

            BEE_CHECK(ctrl->winner.has_value());
            return std::move(*ctrl->winner);
        }
    };

} // namespace detail

// =========================================================================
// when_any() 公开 API
// =========================================================================

/// 同构 when_any — 竞争等待第一个完成的 Task。
/// 所有 Task 都会运行至完成（结构化并发保证）。
/// 获胜者结果返回，其余结果丢弃。
template <typename T>
auto when_any(std::vector<Task<T>> tasks) -> Task<WhenAnyResult<T>>
{
    BEE_CHECK(!tasks.empty());

    auto ctrl = std::make_shared<detail::WhenAnyControl<T>>(tasks.size());
    co_return co_await detail::WhenAnyAwaiter<T>{ctrl, tasks};
}

} // namespace bee
