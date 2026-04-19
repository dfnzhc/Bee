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

    // ── promise 返回值基类 ──────────────────────────────────────────────────
    //
    // MSVC 要求 return_value 与 return_void 出现在不同类型中（不能用
    // requires 约束在同一类内分支），这里通过基类特化拆分。

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

    // ── TaskFinalAwaiter ───────────────────────────────────────────────────
    //
    // 协程到达 final_suspend 时的收尾逻辑，分两条路径：
    //
    //   · 有 continuation（被 co_await）：仅发布终态后做对称转移，不触碰
    //     信号量。等待方是协程，它会在自身 final_suspend 时再处理。
    //
    //   · 无 continuation（被 get/wait 阻塞等待）：先拷贝一份 shared_ptr
    //     持有的信号量到本地保活，再发布终态 + release 信号量。这样即使
    //     等待方在 acquire() 返回后立刻销毁协程帧（含 promise），信号量
    //     内部的 `WakeByAddress`/原子操作仍运作在我们本地持有的对象上，
    //     不会出现 use-after-free。
    //
    // 关键：await_suspend 必须返回 `void` 而非 `coroutine_handle<>`。
    // 实测 MSVC 在 await_suspend 返回 coroutine_handle<> 时，会把返回值
    // 临时对象存放于协程帧中。一旦我们在 await_suspend 内 release 信号
    // 量，等待方即可立刻销毁该帧，而编译器后续构造返回值的写入就会命中
    // 已释放内存（ASan 可稳定复现 heap-use-after-free，其栈指向
    // `std::coroutine_handle<>::from_address` 的构造）。
    // 返回 void 时 MSVC 不再在帧内写入返回值，UAF 消失。
    //
    // 继承 continuation 路径因此不再使用对称转移，而是 eager-resume。
    // 这会让协程调用链在当前线程上线性展开；我们的调度模型下链路通常
    // 只有 2~3 层，可接受。
    //
    // 注：get/wait 不再依据 task_state 做"是否需要 acquire"的短路判断，
    // 这是为了消除"state 已发布但 release 尚未调用"窗口造成的 UAF。

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

            // 必须先取走 continuation：一旦发布终态/释放信号量，promise
            // 就可能被等待方销毁。
            auto cont        = promise.continuation;
            auto final_state = promise.exception ? TaskState::Failed : TaskState::Completed;

            if (cont) {
                // 协程等待者：发布终态后 eager-resume。
                promise.task_state.store(final_state, std::memory_order_release);
                cont.resume();
                return;
            }

            // 阻塞等待者：
            //   1. 将 shared_ptr<EventCount> 拷贝到本地栈，锁住其生命周期；
            //      这样即便等待线程在 wait 返回后立刻销毁协程帧，本线程
            //      仍可安全完成 notify_all（EventCount 对象仍活着）；
            //   2. 发布终态；
            //   3. 通过本地句柄 notify_all；
            //   4. 返回 void —— MSVC 生成的后续代码不再写入协程帧，等待
            //      方可立刻销毁帧而不会 UAF。
            auto ec = promise.ready;
            promise.task_state.store(final_state, std::memory_order_release);
            if (ec) {
                ec->notify_all();
            }
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
 *   · binary_semaphore `ready` 用于 get/wait 的阻塞等待，由 final_suspend
 *     在"无 continuation"路径上释放。
 *   · co_await 路径使用对称转移，不参与信号量。
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

        auto& promise = handle_.promise();
        // 本地保活：确保 EventCount 至少活到本函数返回；这样即便 worker
        // 尚在执行 notify_all，本地拷贝也能让其安全完成。
        auto  ec      = promise.ready;

        // 使用 EventCount::await 的 check-prepare-check-wait 语义等待终态。
        // 与旧 binary_semaphore 不同，EventCount 支持多次 notify 且允许在
        // 已就绪后立即短路（无需"先 acquire 一次再 release"的对称配对）。
        ec->await([&] {
            auto s = promise.task_state.load(std::memory_order_acquire);
            return s == TaskState::Completed || s == TaskState::Failed;
        });

        // 提取异常指针后再处理结果，避免异常路径下未定义顺序。
        auto ex = promise.exception;

        if constexpr (!std::is_void_v<T>) {
            std::optional<T> tmp;
            if (!ex && promise.result.has_value()) {
                tmp.emplace(std::move(*promise.result));
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

        auto& promise = handle_.promise();
        auto  ec      = promise.ready; // 本地保活
        ec->await([&] {
            auto s = promise.task_state.load(std::memory_order_acquire);
            return s == TaskState::Completed || s == TaskState::Failed;
        });

        auto ex = promise.exception;
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

        auto& promise = handle_.promise();
        auto  ec      = promise.ready; // 本地保活
        (void)ec;

        if (is_ready()) {
            return promise.task_state.load(std::memory_order_acquire);
        }

        const auto deadline = std::chrono::steady_clock::now() + timeout;
        constexpr auto poll_interval = std::chrono::milliseconds{1};
        while (!is_ready()) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) break;
            const auto remaining = deadline - now;
            std::this_thread::sleep_for(remaining < poll_interval ? remaining : poll_interval);
        }

        return promise.task_state.load(std::memory_order_acquire);
    }

    // ── 非阻塞查询 ──────────────────────────────────────────────────────────

    [[nodiscard]] auto is_ready() const noexcept -> bool
    {
        if (!handle_) return false;
        auto s = handle_.promise().task_state.load(std::memory_order_acquire);
        return s == TaskState::Completed || s == TaskState::Failed;
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

                // co_await 仅允许在未启动的 Task 上调用：若已被 get/wait 启动，
                // continuation 与 FinalAwaiter 之间无同步，存在 TOCTOU 双重恢复竞态。
                bool expected = false;
                BEE_CHECK(promise.started.compare_exchange_strong(
                    expected, true, std::memory_order_acq_rel));

                promise.continuation = caller;
                promise.task_state.store(TaskState::Running, std::memory_order_release);
                return handle; // 对称转移：开始执行内层 Task
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
        bool expected = false;
        if (promise.started.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            promise.task_state.store(TaskState::Running, std::memory_order_release);
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
    // 状态机
    std::atomic<TaskState> task_state{TaskState::Pending};
    std::exception_ptr     exception;

    // 同步原语
    std::coroutine_handle<> continuation{nullptr};
    // 同步原语放入 shared_ptr 中，使得 worker 在 final_suspend 时可本地保活，
    // 避免 waiter 销毁协程帧后 notify_all() 内部访问悬垂内存。
    //
    // 当前为非 lazy 分配（每个 Task 一次 heap 分配）。后续若需要进一步优化，
    // 可在此改为 atomic<shared_ptr<EventCount>> + double-check 模式。
    std::shared_ptr<EventCount> ready{std::make_shared<EventCount>()};
    std::atomic<bool>                      started{false};

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
        exception = std::current_exception();
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
