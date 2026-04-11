/**
 * @File Task.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include <chrono>
#include <memory>
#include <stop_token>
#include <variant>
#include <stdexcept>
#include <type_traits>

#include "Base/Diagnostics/Check.hpp"
#include "Task/Core/SharedState.hpp"

namespace bee
{

// 前向声明，用于友元访问。
class ThreadPool;

template <typename T>
class Task;

template <typename Fn>
auto submit(ThreadPool& pool, Fn&& fn) -> Task<std::invoke_result_t<Fn>>;

template <typename Fn>
auto submit(ThreadPool& pool, Fn&& fn, std::stop_token token) -> Task<std::invoke_result_t<Fn>>;

template <typename Fn>
    requires std::is_invocable_v<Fn, std::stop_token>
auto submit_cancellable(ThreadPool& pool, Fn&& fn, std::stop_source& source) -> Task<std::invoke_result_t<Fn, std::stop_token>>;

// =========================================================================
// Task<T> — 异步结果的轻量句柄
// =========================================================================

template <typename T>
class Task
{
public:
    /// value_type 对 void 使用 std::monostate，
    /// 使 std::tuple<value_type...> 在 when_all 中总是合法。
    using value_type = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

    Task() = default;

    explicit Task(std::shared_ptr<detail::SharedState<T>> state)
        : state_(std::move(state))
    {
    }

    Task(Task&& other) noexcept                    = default;
    auto operator=(Task&& other) noexcept -> Task& = default;

    Task(const Task&)                    = delete;
    auto operator=(const Task&) -> Task& = delete;

    // -----------------------------------------------------------------
    // 阻塞结果访问
    // -----------------------------------------------------------------

    /// 阻塞直到结果可用并返回。
    /// 失败时重新抛出异常；取消时抛出 std::runtime_error。
    /// 非 void 类型的 T 为一次性操作（结果被移走）。
    auto get() -> T
    {
        BEE_CHECK(state_ != nullptr);
        state_->wait();

        auto s = state_->state.load(std::memory_order_acquire);
        if (s == TaskState::Failed) {
            std::rethrow_exception(state_->exception);
        }
        if (s == TaskState::Cancelled) {
            throw std::runtime_error("Task was cancelled");
        }
        BEE_CHECK(s == TaskState::Completed);

        if constexpr (!std::is_void_v<T>) {
            return std::move(*state_->result);
        }
    }

    /// 阻塞直到进入终态。
    auto wait() const -> void
    {
        BEE_CHECK(state_ != nullptr);
        state_->wait();
    }

    /// 限时等待。超时或达到终态时返回当前状态。
    template <typename Rep, typename Period>
    [[nodiscard]] auto wait_for(std::chrono::duration<Rep, Period> timeout) const -> TaskState
    {
        BEE_CHECK(state_ != nullptr);
        return state_->wait_for(timeout);
    }

    // -----------------------------------------------------------------
    // 非阻塞查询
    // -----------------------------------------------------------------

    /// 任务是否处于终态？
    [[nodiscard]] auto is_ready() const noexcept -> bool
    {
        return state_ && state_->is_terminal();
    }

    /// 当前状态。
    [[nodiscard]] auto state() const noexcept -> TaskState
    {
        if (!state_) {
            return TaskState::Pending;
        }
        return state_->state.load(std::memory_order_acquire);
    }

    /// 是否持有有效的共享状态。
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return state_ != nullptr;
    }

    // -----------------------------------------------------------------
    // Continuation
    // -----------------------------------------------------------------

    /// 挂载内联 Continuation — fn 在完成当前任务的线程上直接执行。
    /// 失败/取消时 fn 不执行，错误自动传播到返回的 Task。
    /// 每个 Task 最多调用一次 then()（BEE_CHECK 强制保证）。
    template <typename Fn>
    auto then(Fn&& fn) -> Task<detail::ContinuationResult_t<T, Fn>>
    {
        using R = detail::ContinuationResult_t<T, Fn>;
        BEE_CHECK(state_ != nullptr);

        auto next_state = std::make_shared<detail::SharedState<R>>();
        auto prev_state = state_;

        auto meta = [next = next_state, fn = std::forward<Fn>(fn), prev = prev_state]() mutable {
            auto s = prev->state.load(std::memory_order_acquire);
            if (s == TaskState::Completed) {
                try {
                    detail::invoke_continuation<T, R>(fn, prev.get(), next.get());
                } catch (...) {
                    next->fail(std::current_exception());
                }
            } else if (s == TaskState::Failed) {
                next->fail(prev->exception);
            } else {
                next->cancel();
            }
        };

        bool run_now = false;
        {
            std::lock_guard lock(state_->mutex);
            BEE_CHECK(!state_->has_continuation);
            if (state_->is_terminal()) {
                run_now = true;
            } else {
                state_->continuation     = MoveOnlyFunction<void()>(std::move(meta));
                state_->has_continuation = true;
            }
        }

        if (run_now) {
            meta();
        }

        return Task<R>(next_state);
    }

    /// 挂载池派发 Continuation — 当前任务完成时将 fn 投递到线程池执行。
    /// Pool 参数为模板类型，避免 Task.hpp 对 ThreadPool.hpp 的硬依赖。
    template <typename Pool, typename Fn>
    auto then(Pool& pool, Fn&& fn) -> Task<detail::ContinuationResult_t<T, Fn>>
    {
        using R = detail::ContinuationResult_t<T, Fn>;
        BEE_CHECK(state_ != nullptr);

        auto next_state = std::make_shared<detail::SharedState<R>>();
        auto prev_state = state_;

        auto meta = [next = next_state, fn = std::forward<Fn>(fn), prev = prev_state, &pool]() mutable {
            auto s = prev->state.load(std::memory_order_acquire);
            if (s == TaskState::Completed) {
                pool.post([next, fn = std::move(fn), prev]() mutable {
                    try {
                        detail::invoke_continuation<T, R>(fn, prev.get(), next.get());
                    } catch (...) {
                        next->fail(std::current_exception());
                    }
                });
            } else if (s == TaskState::Failed) {
                next->fail(prev->exception);
            } else {
                next->cancel();
            }
        };

        bool run_now = false;
        {
            std::lock_guard lock(state_->mutex);
            BEE_CHECK(!state_->has_continuation);
            if (state_->is_terminal()) {
                run_now = true;
            } else {
                state_->continuation     = MoveOnlyFunction<void()>(std::move(meta));
                state_->has_continuation = true;
            }
        }

        if (run_now) {
            meta();
        }

        return Task<R>(next_state);
    }

private:
    std::shared_ptr<detail::SharedState<T>> state_;

    // 授予创建或组合 Task 的自由函数访问权限。
    template <typename Fn>
    friend auto submit(ThreadPool& pool, Fn&& fn) -> Task<std::invoke_result_t<Fn>>;

    template <typename Fn>
    friend auto submit(ThreadPool& pool, Fn&& fn, std::stop_token token) -> Task<std::invoke_result_t<Fn>>;

    template <typename Fn>
        requires std::is_invocable_v<Fn, std::stop_token>
    friend auto submit_cancellable(ThreadPool& pool, Fn&& fn, std::stop_source& source) -> Task<std::invoke_result_t<Fn, std::stop_token>>;

    template <typename U>
    friend class Task;
};

} // namespace bee
