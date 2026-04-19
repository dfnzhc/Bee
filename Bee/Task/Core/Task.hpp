/**
 * @File Task.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief Task<T> —— 惰性协程返回类型与异步结果句柄。
 */

#pragma once

#include <atomic>
#include <chrono>
#include <coroutine>
#include <exception>
#include <memory>
#include <optional>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Base/Core/Defines.hpp"
#include "Base/Diagnostics/Check.hpp"
#include "Base/Sync/EventCount.hpp"
#include "Task/Core/TaskState.hpp"

namespace bee
{

// ── 前向声明 ────────────────────────────────────────────────────────────────

template <typename T>
class Task;

template <typename T>
struct WhenAnyResult;

template <typename... Ts>
auto when_all(Task<Ts>&&... tasks) -> Task<std::tuple<typename Task<Ts>::value_type...>>;

template <typename T>
auto when_all(std::vector<Task<T>> tasks) -> Task<std::vector<T>>;

template <typename T>
auto when_any(std::vector<Task<T>> tasks) -> Task<WhenAnyResult<T>>;

// ===========================================================================
// detail —— 内部辅助类型
// ===========================================================================

namespace detail
{

    // ── continuation 返回类型推导 ────────────────────────────────────────────

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

    // ── TaskSharedState ─────────────────────────────────────────────────────
    //
    // Task 的"外置状态块"：所有需要跨协程帧/跨线程观察的字段集中在此。
    // 协程 promise 只持一枚 `shared_ptr<TaskSharedState>`，从而：
    //
    //   · 协程帧可以被 worker 在 final_suspend 阶段安全自毁（帧自毁后，
    //     waiter 读结果仍走 shared_ptr 指向的堆对象，彻底规避 MSVC 在
    //     "帧写返回值"窗口内的 UAF）；
    //   · Task<T>::get/wait 不再必须持有 handle；
    //   · 为后续 Step B 的 FinalAwaiter 双路径（continuation 分支走
    //     symmetric transfer，waiter 分支 worker 自毁 + noop）提供前置。
    //
    // 继续遵守不变式 A：该对象由 promise 持有一份、Task<T>（及 await 者）
    // 持有一份；最后一个释放 shared_ptr 的人负责 delete。EventCount 本身
    // 也跟随 SharedState 生命周期，不再需要单独 shared_ptr<EventCount>。

    template <typename T>
    struct TaskSharedStateBase
    {
        std::atomic<TaskState>  task_state{TaskState::Pending};
        std::exception_ptr      exception;
        EventCount              ready;
        std::coroutine_handle<> continuation{nullptr}; // co_await 路径下由 awaiter 写入
    };

    template <typename T>
    struct TaskSharedState : TaskSharedStateBase<T>
    {
        std::optional<T> result;
    };

    template <>
    struct TaskSharedState<void> : TaskSharedStateBase<void>
    {
    };

    // ── promise 返回值基类 ──────────────────────────────────────────────────
    //
    // MSVC 要求 return_value 与 return_void 出现在不同类型中（不能用
    // requires 约束在同一类内分支），这里通过基类特化拆分。
    // 结果直接写入 shared_ptr<TaskSharedState>，而不再保存在 promise 内。

    template <typename T>
    struct TaskPromiseReturn
    {
        std::shared_ptr<TaskSharedState<T>> state{std::make_shared<TaskSharedState<T>>()};

        auto return_value(T value) -> void
        {
            state->result.emplace(std::move(value));
        }
    };

    template <>
    struct TaskPromiseReturn<void>
    {
        std::shared_ptr<TaskSharedState<void>> state{std::make_shared<TaskSharedState<void>>()};

        auto return_void() -> void
        {
        }
    };

    // ── TaskFinalAwaiter ───────────────────────────────────────────────────
    //
    // 协程到达 final_suspend 时的收尾逻辑。
    //
    // 当前（Step A）仍维持"返回 void + eager resume/notify"的保守形态，
    // 所有终态 / 异常 / 结果均通过 shared_ptr<TaskSharedState> 发布，因此
    // waiter 即便在 notify_all 后立即销毁协程帧，worker 后续代码也不会
    // 访问已释放的帧内存——它访问的是 shared_ptr 指向的堆对象。
    //
    //   · 有 continuation（被 co_await）：发布终态后 eager resume，不触
    //     碰 EventCount；continuation 协程自己负责最终读取结果/异常。
    //
    //   · 无 continuation（被 get/wait 阻塞等待）：发布终态 + notify_all。
    //     通过 shared_ptr 保活 state，EventCount 不会在 notify 路径中被释放。

    struct TaskFinalAwaiter
    {
        [[nodiscard]] auto await_ready() const noexcept -> bool
        {
            return false;
        }

        template <typename Promise>
        auto await_suspend(std::coroutine_handle<Promise> h) noexcept -> void
        {
            auto& promise = h.promise();
            auto& state   = *promise.state;

            auto cont        = state.continuation;
            auto final_state = state.exception ? TaskState::Failed : TaskState::Completed;

            if (cont) {
                state.task_state.store(final_state, std::memory_order_release);
                cont.resume();
                return;
            }

            // 本地保活 state（防止 waiter 在 notify 前后销毁其 shared_ptr
            // 引用即释放 state 对象——虽然 promise 也持一份，但这里
            // 显式再握一份以表达"notify_all 在本地对象上完成"的不变式）。
            auto state_keep = promise.state;
            state.task_state.store(final_state, std::memory_order_release);
            state.ready.notify_all();
            (void)state_keep;
        }

        auto await_resume() noexcept -> void
        {
        }
    };

    // ── DetachedTask ───────────────────────────────────────────────────────
    //
    // 即发即弃协程，被 when_all/when_any/AsyncScope 内部用作 wrapper：
    //   · initial_suspend = suspend_never  立即开跑
    //   · final_suspend   = suspend_never  自动销毁帧
    // 异常需在 wrapper 内部接住转存到控制块；逃逸到此即视为逻辑 bug。

    struct DetachedTask
    {
        struct promise_type
        {
            auto get_return_object() -> DetachedTask { return {}; }
            auto initial_suspend() noexcept -> std::suspend_never { return {}; }
            auto final_suspend() noexcept -> std::suspend_never { return {}; }
            auto return_void() -> void {}

            auto unhandled_exception() -> void
            {
                BEE_ASSERT(false && "DetachedTask: 异常逃逸，组合器内部逻辑有 bug");
            }
        };
    };

    // ── then() 实现前向声明（定义在 Task<T> 之后）───────────────────────────

    template <typename T, typename Fn>
    auto then_impl(Task<T> pred, Fn fn) -> Task<ContinuationResult_t<T, Fn>>;

    template <typename T, typename S, typename Fn>
    auto then_impl_scheduled(Task<T> pred, S& scheduler, Fn fn) -> Task<ContinuationResult_t<T, Fn>>;

} // namespace detail

// ===========================================================================
// Task<T> —— 惰性协程返回类型 / 异步结果句柄
// ===========================================================================

/**
 * @brief 惰性协程返回类型，同时充当异步结果句柄。
 *
 * 生命周期约定：
 *   · 创建时不执行（initial_suspend = suspend_always）。
 *   · 通过 co_await / get() / wait() 启动；start_if_needed 是幂等的。
 *   · 未启动的 Task 可安全析构；已启动的 Task 必须等待至终态后销毁。
 *   · get() / wait() 是一次性消费操作：返回后 handle 即被销毁。
 *
 * 同步原语：
 *   · EventCount `state->ready` 用于 get/wait 的阻塞等待，由 final_suspend
 *     在"无 continuation"路径上 notify_all。
 *   · co_await 路径使用 eager resume，不参与信号量。
 *   · 所有终态字段（result / exception / task_state）均外置于
 *     `shared_ptr<TaskSharedState>`：即便 waiter 早于 worker 销毁协程帧，
 *     worker 后续代码仍运作在堆对象上，不会产生 UAF。
 */
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

    // ── 阻塞访问 ────────────────────────────────────────────────────────────

    /// 启动（若尚未启动）并阻塞至完成。返回结果或重抛异常。
    /// 调用后 Task 句柄被销毁；不可再次访问。
    auto get() -> T
    {
        BEE_CHECK(handle_ != nullptr);
        start_if_needed();

        auto& state = *handle_.promise().state;
        // state 已经是 shared_ptr 引用——本函数通过 promise 的 shared_ptr
        // 访问；下面的 destroy_if_valid() 会释放协程帧，但 state 由
        // 本地拷贝的 shared_ptr 保活。
        auto state_keep = handle_.promise().state;

        state.ready.await([&] {
            auto s = state.task_state.load(std::memory_order_acquire);
            return s == TaskState::Completed || s == TaskState::Failed;
        });

        auto ex = state.exception;

        if constexpr (!std::is_void_v<T>) {
            std::optional<T> tmp;
            if (!ex && state.result.has_value()) {
                tmp.emplace(std::move(*state.result));
            }
            destroy_if_valid();
            if (ex) {
                std::rethrow_exception(ex);
            }
            return std::move(*tmp);
        }
        else {
            destroy_if_valid();
            if (ex) {
                std::rethrow_exception(ex);
            }
        }
    }

    /// 启动并阻塞至终态。失败时重抛异常。
    /// 一次性消费：调用后 Task 句柄被销毁。
    auto wait() -> void
    {
        BEE_CHECK(handle_ != nullptr);
        start_if_needed();

        auto& state      = *handle_.promise().state;
        auto  state_keep = handle_.promise().state;

        state.ready.await([&] {
            auto s = state.task_state.load(std::memory_order_acquire);
            return s == TaskState::Completed || s == TaskState::Failed;
        });

        auto ex = state.exception;
        destroy_if_valid();
        if (ex) {
            std::rethrow_exception(ex);
        }
    }

    /// 限时等待，返回当前状态。**不消费 Task**，超时后可继续 get/wait。
    ///
    /// 合约：
    ///   · 同一 Task 允许并发 wait_for（EventCount 支持多 waiter）。
    ///   · 不允许在已被 co_await 的 Task 上调用（co_await 已转移 handle）。
    ///
    /// 实现说明：EventCount 不直接支持 timed-wait，此处采用轮询 +
    /// sleep_for 的保守策略（响应延迟 <= 1ms）。该路径非热路径，可接受。
    [[nodiscard]] auto wait_for(std::chrono::nanoseconds timeout) -> TaskState
    {
        BEE_CHECK(handle_ != nullptr);
        start_if_needed();

        auto& state = *handle_.promise().state;

        if (is_ready()) {
            return state.task_state.load(std::memory_order_acquire);
        }

        const auto     deadline      = std::chrono::steady_clock::now() + timeout;
        constexpr auto poll_interval = std::chrono::milliseconds{1};
        while (!is_ready()) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) break;
            const auto remaining = deadline - now;
            std::this_thread::sleep_for(remaining < poll_interval ? remaining : poll_interval);
        }

        return state.task_state.load(std::memory_order_acquire);
    }

    // ── 非阻塞查询 ──────────────────────────────────────────────────────────

    [[nodiscard]] auto is_ready() const noexcept -> bool
    {
        if (!handle_) return false;
        auto s = handle_.promise().state->task_state.load(std::memory_order_acquire);
        return s == TaskState::Completed || s == TaskState::Failed;
    }

    [[nodiscard]] auto state() const noexcept -> TaskState
    {
        if (!handle_) return TaskState::Pending;
        return handle_.promise().state->task_state.load(std::memory_order_acquire);
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    // ── co_await 支持 ───────────────────────────────────────────────────────

    auto operator co_await() &&
    {
        // TaskAwaiter 拥有 handle 的所有权，并在析构时销毁协程帧。
        // 这避免了原实现中 await_resume 后无人 destroy 导致的协程帧泄漏。
        struct TaskAwaiter
        {
            handle_type handle;

            explicit TaskAwaiter(handle_type h) noexcept : handle(h) {}

            TaskAwaiter(TaskAwaiter&& other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

            TaskAwaiter(const TaskAwaiter&)                    = delete;
            auto operator=(const TaskAwaiter&) -> TaskAwaiter& = delete;
            auto operator=(TaskAwaiter&&) -> TaskAwaiter&      = delete;

            ~TaskAwaiter()
            {
                if (handle) {
                    handle.destroy();
                }
            }

            [[nodiscard]] auto await_ready() const noexcept -> bool
            {
                return false;
            }

            auto await_suspend(std::coroutine_handle<> caller) -> std::coroutine_handle<>
            {
                auto& promise = handle.promise();
                auto& state   = *promise.state;

                // co_await 仅允许在未启动的 Task 上调用：若已被 get/wait 启动，
                // continuation 与 FinalAwaiter 之间无同步，存在 TOCTOU 双重恢复竞态。
                bool expected = false;
                BEE_CHECK(promise.started.compare_exchange_strong(
                    expected, true, std::memory_order_acq_rel));

                state.continuation = caller;
                state.task_state.store(TaskState::Running, std::memory_order_release);
                return handle; // 对称转移：开始执行内层 Task
            }

            auto await_resume() -> T
            {
                auto& state = *handle.promise().state;
                if (state.exception) {
                    std::rethrow_exception(state.exception);
                }

                if constexpr (!std::is_void_v<T>) {
                    BEE_CHECK(state.result.has_value());
                    return std::move(*state.result);
                }
            }
        };

        BEE_CHECK(handle_ != nullptr);
        return TaskAwaiter{std::exchange(handle_, nullptr)};
    }

    // 兼容左值 co_await（仍要求 Task 仅在所属作用域使用一次）。
    auto operator co_await() &
    {
        return std::move(*this).operator co_await();
    }

    // ── continuation ────────────────────────────────────────────────────────

    /// 内联 continuation：fn 在前驱完成的线程上执行。
    template <typename Fn>
    auto then(Fn&& fn) -> Task<detail::ContinuationResult_t<T, Fn>>
    {
        return detail::then_impl<T>(std::move(*this), std::forward<Fn>(fn));
    }

    /// Scheduler 派发 continuation：fn 在 scheduler 工作线程上执行。
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
        bool  expected = false;
        if (promise.started.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            promise.state->task_state.store(TaskState::Running, std::memory_order_release);
            handle_.resume();
        }
    }

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

// ===========================================================================
// promise_type 定义
// ===========================================================================

template <typename T>
struct Task<T>::promise_type : detail::TaskPromiseReturn<T>
{
    // 启动标记（仅此处保留于 promise；其余状态均下放到 SharedState）
    std::atomic<bool> started{false};

    // 协程接口
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
        this->state->exception = std::current_exception();
    }
};

// ===========================================================================
// then() 实现
// ===========================================================================

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
