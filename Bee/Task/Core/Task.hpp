/**
 * @File Task.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <coroutine>
#include <exception>
#include <optional>
#include <semaphore>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Base/Core/Defines.hpp"
#include "Base/Diagnostics/Check.hpp"
#include "Task/Core/TaskState.hpp"

namespace bee
{

// 前向声明
template <typename T>
class Task;

template <typename... Ts>
auto when_all(Task<Ts>&&... tasks) -> Task<std::tuple<typename Task<Ts>::value_type...>>;

template <typename T>
auto when_all(std::vector<Task<T>> tasks) -> Task<std::vector<T>>;

template <typename T>
struct WhenAnyResult;

template <typename T>
auto when_any(std::vector<Task<T>> tasks) -> Task<WhenAnyResult<T>>;

// =========================================================================
// detail — 内部辅助类型
// =========================================================================

namespace detail
{

    // ── Continuation 类型推导 ──

    template <typename T, typename Fn, bool IsVoid = std::is_void_v<T>>
    struct ContinuationResultImpl;

    template <typename T, typename Fn>
    struct ContinuationResultImpl<T, Fn, false>
    {
        using type = std::invoke_result_t<Fn, T>;
    };

    template <typename T, typename Fn>
    struct ContinuationResultImpl<T, Fn, true>
    {
        using type = std::invoke_result_t<Fn>;
    };

    template <typename T, typename Fn>
    using ContinuationResult_t = typename ContinuationResultImpl<T, Fn>::type;

    // ── MSVC 要求 return_value/return_void 在不同类型中 ──

    template <typename T>
    struct TaskPromiseReturn
    {
        std::optional<T> result{};

        auto return_value(T value) -> void
        {
            result.emplace(std::move(value));
        }
    };

    template <>
    struct TaskPromiseReturn<void>
    {
        auto return_void() -> void
        {
        }
    };

    // ── TaskFinalAwaiter：修复 V1 的 UAF 问题 ──
    //
    // V1 bug: done.store(true) 后再访问 promise.continuation，
    // 此时 .get() 调用者可能已销毁协程帧。
    // V2 fix: 在标记终态之前读取 continuation。

    struct TaskFinalAwaiter
    {
        [[nodiscard]] auto await_ready() const noexcept -> bool
        {
            return false;
        }

        template <typename Promise>
        auto await_suspend(std::coroutine_handle<Promise> h) noexcept -> std::coroutine_handle<>
        {
            auto& promise = h.promise();

            // 关键：先读取 continuation，再标记终态
            auto cont = promise.continuation;

            // 确定终态类型
            auto final_state = promise.exception ? TaskState::Failed : TaskState::Completed;
            promise.task_state.store(final_state, std::memory_order_release);

            if (cont) {
                return cont; // 对称转移恢复 co_await 调用者
            }

            // 无协程等待者 — 为 .get()/.wait() 释放信号量
            promise.ready.release();
            return std::noop_coroutine();
        }

        auto await_resume() noexcept -> void
        {
        }
    };

    // ── DetachedTask：即发即弃协程（when_all/when_any/AsyncScope 内部使用）──

    struct DetachedTask
    {
        struct promise_type
        {
            auto get_return_object() -> DetachedTask { return {}; }

            auto initial_suspend() noexcept -> std::suspend_never { return {}; } // 立即启动

            auto final_suspend() noexcept -> std::suspend_never { return {}; } // 自动销毁

            auto return_void() -> void {}

            auto unhandled_exception() -> void {} // 异常已在调用点处理
        };
    };

    // ── then() 实现辅助（前向声明，定义在 Task<T> 之后）──

    template <typename T, typename Fn>
    auto then_impl(Task<T> pred, Fn fn) -> Task<ContinuationResult_t<T, Fn>>;

    template <typename T, typename S, typename Fn>
    auto then_impl_scheduled(Task<T> pred, S& scheduler, Fn fn) -> Task<ContinuationResult_t<T, Fn>>;

} // namespace detail

// =========================================================================
// Task<T> — 统一惰性协程
// =========================================================================

/// Task<T> 是一个惰性协程返回类型，同时也是异步结果句柄。
///
/// 创建时不执行（initial_suspend = suspend_always），
/// 通过 co_await / .get() / .wait() 启动。
///
/// 生命周期规则：
/// - 未启动的 Task 可安全析构
/// - 已启动的 Task 必须在析构前完成（通过 get/wait/co_await/AsyncScope）
/// - .get() 和 .wait() 是一次性操作
template <typename T = void>
class [[nodiscard]] Task
{
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    using value_type  = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

    Task() = default;

    explicit Task(handle_type h) noexcept : handle_(h)
    {
    }

    Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, nullptr))
    {
    }

    auto operator=(Task&& other) noexcept -> Task&
    {
        if (this != &other) {
            destroy_if_valid();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    ~Task()
    {
        destroy_if_valid();
    }

    Task(const Task&)                    = delete;
    auto operator=(const Task&) -> Task& = delete;

    // ── 阻塞访问 ──

    /// 启动（若未启动）并阻塞直到完成。返回结果或重抛异常。
    /// 一次性消费操作：调用后 Task 变为空（handle 销毁）。
    auto get() -> T
    {
        BEE_CHECK(handle_ != nullptr);
        start_if_needed();

        auto& promise = handle_.promise();

        // 若已完成无需等待
        if (!is_ready()) {
            promise.ready.acquire();
        }

        // 提取结果前先保存异常指针，确保析构正确
        auto ex = promise.exception;

        if constexpr (!std::is_void_v<T>) {
            std::optional<T> result_val;
            if (!ex && promise.result.has_value()) {
                result_val.emplace(std::move(*promise.result));
            }
            // 消费完毕：销毁协程帧
            destroy_if_valid();
            if (ex) {
                std::rethrow_exception(ex);
            }
            return std::move(*result_val);
        }
        else {
            destroy_if_valid();
            if (ex) {
                std::rethrow_exception(ex);
            }
        }
    }

    /// 启动并阻塞直到终态。失败/取消时重抛异常。
    /// 一次性消费操作：调用后 Task 变为空。
    auto wait() -> void
    {
        BEE_CHECK(handle_ != nullptr);
        start_if_needed();

        auto& promise = handle_.promise();
        if (!is_ready()) {
            promise.ready.acquire();
        }

        auto ex = promise.exception;
        destroy_if_valid();
        if (ex) {
            std::rethrow_exception(ex);
        }
    }

    /// 限时等待。返回当前状态。
    /// 注意：不消费 Task。超时返回当前状态（可能是 Running）。
    [[nodiscard]] auto wait_for(std::chrono::nanoseconds timeout) -> TaskState
    {
        BEE_CHECK(handle_ != nullptr);
        start_if_needed();

        auto& promise = handle_.promise();
        if (is_ready()) {
            return promise.task_state.load(std::memory_order_acquire);
        }

        // 使用 try_acquire_for 实现限时等待
        if (promise.ready.try_acquire_for(timeout)) {
            // 成功获取 → 归还信号量供后续 get()/wait() 使用
            promise.ready.release();
            return promise.task_state.load(std::memory_order_acquire);
        }

        return promise.task_state.load(std::memory_order_acquire);
    }

    // ── 非阻塞查询 ──

    [[nodiscard]] auto is_ready() const noexcept -> bool
    {
        if (!handle_) return false;
        auto s = handle_.promise().task_state.load(std::memory_order_acquire);
        return s == TaskState::Completed || s == TaskState::Failed || s == TaskState::Cancelled;
    }

    [[nodiscard]] auto state() const noexcept -> TaskState
    {
        if (!handle_) return TaskState::Pending;
        return handle_.promise().task_state.load(std::memory_order_acquire);
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    // ── 协程支持：co_await Task<T> ──

    auto operator co_await()
    {
        struct TaskAwaiter
        {
            handle_type handle;

            [[nodiscard]] auto await_ready() const noexcept -> bool
            {
                return false;
            }

            auto await_suspend(std::coroutine_handle<> caller) -> std::coroutine_handle<>
            {
                auto& promise = handle.promise();

                // co_await 仅允许在未启动的 Task 上使用。
                // 若 Task 已被 get()/wait()/wait_for() 启动，co_await 存在
                // TOCTOU 双重恢复竞态（continuation 设置与 FinalAwaiter 读取无同步）。
                bool expected = false;
                BEE_CHECK(promise.started.compare_exchange_strong(expected, true, std::memory_order_acq_rel));

                promise.continuation = caller;
                promise.task_state.store(TaskState::Running, std::memory_order_release);
                return handle; // 对称转移：开始执行此 Task
            }

            auto await_resume() -> T
            {
                auto& promise = handle.promise();
                if (promise.exception) {
                    std::rethrow_exception(promise.exception);
                }

                if constexpr (!std::is_void_v<T>) {
                    BEE_CHECK(promise.result.has_value());
                    return std::move(*promise.result);
                }
            }
        };

        BEE_CHECK(handle_ != nullptr);
        return TaskAwaiter{std::exchange(handle_, nullptr)};
    }

    // ── Continuation ──

    /// 内联 continuation — 在完成线程上执行。
    template <typename Fn>
    auto then(Fn&& fn) -> Task<detail::ContinuationResult_t<T, Fn>>
    {
        return detail::then_impl<T>(std::move(*this), std::forward<Fn>(fn));
    }

    /// Scheduler 派发 continuation — fn 在 scheduler 线程上执行。
    template <typename S, typename Fn>
    auto then(S& scheduler, Fn&& fn) -> Task<detail::ContinuationResult_t<T, Fn>>
    {
        return detail::then_impl_scheduled<T>(std::move(*this), scheduler, std::forward<Fn>(fn));
    }

private:
    handle_type handle_{nullptr};

    auto destroy_if_valid() -> void
    {
        if (handle_) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

    auto start_if_needed() -> void
    {
        auto& promise = handle_.promise();
        bool expected  = false;
        if (promise.started.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            promise.task_state.store(TaskState::Running, std::memory_order_release);
            handle_.resume();
        }
    }

    // 友元声明
    template <typename U>
    friend class Task;

    template <typename... Ts>
    friend auto when_all(Task<Ts>&&... tasks) -> Task<std::tuple<typename Task<Ts>::value_type...>>;

    template <typename U>
    friend auto when_all(std::vector<Task<U>> tasks) -> Task<std::vector<U>>;

    template <typename U>
    friend auto when_any(std::vector<Task<U>> tasks) -> Task<WhenAnyResult<U>>;

    template <typename U, typename Fn>
    friend auto detail::then_impl(Task<U> pred, Fn fn) -> Task<detail::ContinuationResult_t<U, Fn>>;

    template <typename U, typename S, typename Fn>
    friend auto detail::then_impl_scheduled(Task<U> pred, S& scheduler, Fn fn)
        -> Task<detail::ContinuationResult_t<U, Fn>>;

};

// =========================================================================
// promise_type 定义
// =========================================================================

template <typename T>
struct Task<T>::promise_type : detail::TaskPromiseReturn<T>
{
    // ── 状态 ──
    std::atomic<TaskState> task_state{TaskState::Pending};
    std::exception_ptr     exception;

    // ── 同步 ──
    std::coroutine_handle<> continuation{nullptr};
    std::binary_semaphore   ready{0};
    std::atomic<bool>       started{false};

    // ── 协程接口 ──

    auto get_return_object() -> Task
    {
        return Task{handle_type::from_promise(*this)};
    }

    auto initial_suspend() noexcept -> std::suspend_always
    {
        return {}; // 惰性：创建即挂起
    }

    auto final_suspend() noexcept -> detail::TaskFinalAwaiter
    {
        return {};
    }

    auto unhandled_exception() -> void
    {
        exception = std::current_exception();
    }
};

// =========================================================================
// then() 实现
// =========================================================================

namespace detail
{

    template <typename T, typename Fn>
    auto then_impl(Task<T> pred, Fn fn) -> Task<ContinuationResult_t<T, Fn>>
    {
        using R = ContinuationResult_t<T, Fn>;

        if constexpr (std::is_void_v<T> && std::is_void_v<R>) {
            co_await std::move(pred);
            fn();
        }
        else if constexpr (std::is_void_v<T>) {
            co_await std::move(pred);
            co_return fn();
        }
        else if constexpr (std::is_void_v<R>) {
            T val = co_await std::move(pred);
            fn(std::move(val));
        }
        else {
            T val = co_await std::move(pred);
            co_return fn(std::move(val));
        }
    }

    template <typename T, typename S, typename Fn>
    auto then_impl_scheduled(Task<T> pred, S& scheduler, Fn fn) -> Task<ContinuationResult_t<T, Fn>>
    {
        using R = ContinuationResult_t<T, Fn>;

        if constexpr (std::is_void_v<T> && std::is_void_v<R>) {
            co_await std::move(pred);
            co_await scheduler.schedule();
            fn();
        }
        else if constexpr (std::is_void_v<T>) {
            co_await std::move(pred);
            co_await scheduler.schedule();
            co_return fn();
        }
        else if constexpr (std::is_void_v<R>) {
            T val = co_await std::move(pred);
            co_await scheduler.schedule();
            fn(std::move(val));
        }
        else {
            T val = co_await std::move(pred);
            co_await scheduler.schedule();
            co_return fn(std::move(val));
        }
    }

} // namespace detail

} // namespace bee
