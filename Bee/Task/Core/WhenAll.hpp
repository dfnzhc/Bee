/**
 * @File WhenAll.hpp
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
#include <mutex>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Task/Core/Task.hpp"

namespace bee
{

namespace detail
{

    // =========================================================================
    // Variadic when_all 内部结构
    // =========================================================================

    /// variadic when_all 的控制块。
    /// 使用 "+1 trick"：remaining 初始化为 N+1，parent 投出最后一票来消除竞态。
    template <typename... Ts>
    struct WhenAllControl
    {
        using ResultType = std::tuple<typename Task<Ts>::value_type...>;

        std::atomic<std::size_t> remaining;
        std::coroutine_handle<>  parent{nullptr};
        ResultType               results{};
        std::exception_ptr       first_ex;
        std::mutex               ex_mutex;

        explicit WhenAllControl(std::size_t n) : remaining(n + 1)
        {
        }
    };

    /// 每个子任务的 wrapper 协程（DetachedTask：立即启动，自动销毁）
    template <std::size_t I, typename T, typename ControlPtr>
    auto when_all_wrapper(ControlPtr ctrl, Task<T> task) -> DetachedTask
    {
        try {
            if constexpr (std::is_void_v<T>) {
                co_await std::move(task);
                // value_type 是 monostate，已在 tuple 中默认构造
            }
            else {
                std::get<I>(ctrl->results) = co_await std::move(task);
            }
        }
        catch (...) {
            std::lock_guard lock(ctrl->ex_mutex);
            if (!ctrl->first_ex) {
                ctrl->first_ex = std::current_exception();
            }
        }

        // 最后一个完成的 wrapper 恢复 parent 协程
        if (ctrl->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            ctrl->parent.resume();
        }
    }

    /// variadic when_all 的自定义 awaiter
    template <typename... Ts>
    struct WhenAllAwaiter
    {
        using ControlType = WhenAllControl<Ts...>;
        using ResultType  = typename ControlType::ResultType;

        std::shared_ptr<ControlType> ctrl;
        std::tuple<Task<Ts>...>      tasks;

        explicit WhenAllAwaiter(Task<Ts>&&... ts)
            : ctrl(std::make_shared<ControlType>(sizeof...(Ts)))
            , tasks(std::move(ts)...)
        {
        }

        [[nodiscard]] auto await_ready() const noexcept -> bool
        {
            return sizeof...(Ts) == 0;
        }

        auto await_suspend(std::coroutine_handle<> h) -> bool
        {
            ctrl->parent = h;

            // 启动所有 wrapper 协程
            [this]<std::size_t... Is>(std::index_sequence<Is...>) {
                (when_all_wrapper<Is, Ts>(ctrl, std::move(std::get<Is>(tasks))), ...);
            }(std::index_sequence_for<Ts...>{});

            // 投出 parent 的 "票"
            // 如果所有 wrapper 已完成，remaining 为 1，减后为 0 → 不挂起
            return ctrl->remaining.fetch_sub(1, std::memory_order_acq_rel) != 1;
        }

        auto await_resume() -> ResultType
        {
            if (ctrl->first_ex) {
                std::rethrow_exception(ctrl->first_ex);
            }
            return std::move(ctrl->results);
        }
    };

    // =========================================================================
    // Vector when_all 内部结构
    // =========================================================================

    template <typename T>
    struct VectorWhenAllControl
    {
        std::atomic<std::size_t>      remaining;
        std::coroutine_handle<>       parent{nullptr};
        std::vector<std::optional<T>> results;
        std::exception_ptr            first_ex;
        std::mutex                    ex_mutex;

        explicit VectorWhenAllControl(std::size_t n) : remaining(n + 1), results(n)
        {
        }
    };

    template <typename T>
    auto vector_when_all_wrapper(
        std::shared_ptr<VectorWhenAllControl<T>> ctrl,
        std::size_t index,
        Task<T> task) -> DetachedTask
    {
        try {
            ctrl->results[index].emplace(co_await std::move(task));
        }
        catch (...) {
            std::lock_guard lock(ctrl->ex_mutex);
            if (!ctrl->first_ex) {
                ctrl->first_ex = std::current_exception();
            }
        }

        if (ctrl->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            ctrl->parent.resume();
        }
    }

    template <typename T>
    struct VectorWhenAllAwaiter
    {
        std::shared_ptr<VectorWhenAllControl<T>> ctrl;
        std::vector<Task<T>>&                    tasks;

        [[nodiscard]] auto await_ready() const noexcept -> bool
        {
            return tasks.empty();
        }

        auto await_suspend(std::coroutine_handle<> h) -> bool
        {
            ctrl->parent = h;

            for (std::size_t i = 0; i < tasks.size(); ++i) {
                vector_when_all_wrapper<T>(ctrl, i, std::move(tasks[i]));
            }

            return ctrl->remaining.fetch_sub(1, std::memory_order_acq_rel) != 1;
        }

        auto await_resume() -> std::vector<T>
        {
            if (ctrl->first_ex) {
                std::rethrow_exception(ctrl->first_ex);
            }

            std::vector<T> result;
            result.reserve(ctrl->results.size());
            for (auto& opt : ctrl->results) {
                result.push_back(std::move(*opt));
            }
            return result;
        }
    };

} // namespace detail

// =========================================================================
// when_all() 公开 API
// =========================================================================

/// 异构 when_all — 并发等待所有 Task 完成，返回结果 tuple。
template <typename... Ts>
auto when_all(Task<Ts>&&... tasks) -> Task<std::tuple<typename Task<Ts>::value_type...>>
{
    co_return co_await detail::WhenAllAwaiter<Ts...>(std::move(tasks)...);
}

/// 同构 when_all — 并发等待 vector 中所有 Task 完成，返回结果 vector。
template <typename T>
auto when_all(std::vector<Task<T>> tasks) -> Task<std::vector<T>>
{
    if (tasks.empty()) {
        co_return std::vector<T>{};
    }

    auto ctrl = std::make_shared<detail::VectorWhenAllControl<T>>(tasks.size());
    co_return co_await detail::VectorWhenAllAwaiter<T>{ctrl, tasks};
}

} // namespace bee
