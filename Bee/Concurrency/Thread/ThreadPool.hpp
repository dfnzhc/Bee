/**
 * @File ThreadPool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Base/Core/Defines.hpp"
#include "Base/Diagnostics/Check.hpp"
#include "Base/Core/MoveOnlyFunction.hpp"
#include "Concurrency/LockFree/ChaseLevDeque.hpp"
#include "Concurrency/LockFree/MPMCQueue.hpp"
#include "Concurrency/Threading.hpp"

namespace bee
{

// =============================================================================
// Enums & Configuration
// =============================================================================

/**
 * @brief 线程池关闭策略。
 *
 * - Drain：拒绝新任务，等待已提交任务全部收尾后退出。
 * - Immediate：拒绝新任务，worker 取到排队任务后直接丢弃以加速关闭。
 */
enum class ShutdownMode
{
    Drain,
    Immediate
};

/**
 * @brief 全局队列背压策略。
 *
 * - FailFast：一轮扫描各分片，若全部满则立即失败。
 * - CallerRuns：入队失败时由提交线程直接执行任务。
 * - Block：在超时预算内持续重试，超时后失败。
 */
enum class BackpressurePolicy
{
    FailFast,
    CallerRuns,
    Block
};

/**
 * @brief 线程池生命周期阶段。
 *
 * 状态推进：Running → Quiescing → Draining → Stopping → Stopped。
 */
enum class LifecyclePhase
{
    Running,
    Quiescing,
    Draining,
    Stopping,
    Stopped
};

/**
 * @brief 线程池配置。
 */
struct ThreadPoolConfig
{
    std::size_t               thread_count{std::thread::hardware_concurrency()};
    BackpressurePolicy        backpressure_policy{BackpressurePolicy::FailFast};
    std::chrono::milliseconds enqueue_block_timeout{std::chrono::milliseconds(50)};
};

/**
 * @brief 线程池运行期统计快照。
 *
 * 除 worker_count 外，其他字段均为近似值（读取时不做全局同步）。
 */
struct ThreadPoolStats
{
    std::size_t    worker_count{0};
    LifecyclePhase phase{LifecyclePhase::Running};
    std::size_t    pending_tasks{0};
    std::size_t    active_tasks{0};
    std::uint64_t  submitted_tasks{0};
    std::uint64_t  finalized_tasks{0};
    std::uint64_t  dropped_tasks{0};
    std::uint64_t  local_pop_hits{0};
    std::uint64_t  global_pop_hits{0};
    std::uint64_t  steal_hits{0};
    std::uint64_t  steal_misses{0};
    std::uint64_t  local_queue_overflow_fallbacks{0};
    std::uint64_t  idle_wait_count{0};
    std::uint32_t  global_probe_budget{0};
};

// =============================================================================
// ThreadPool
// =============================================================================

/**
 * @brief 高性能 C++20 线程池（work-stealing + 分片全局队列）。
 *
 * 实现特点：
 * 1. 两级调度：每个 worker 持有本地 Chase-Lev 队列，全局使用分片 MPMC 队列。
 * 2. 取任务顺序：local pop → global pop → steal，兼顾局部性与公平性。
 * 3. 空闲策略：短自旋 + 退避 + 信号量阻塞，降低空转 CPU 占用。
 * 4. 生命周期可观测：提供 LifecyclePhase 与 stats 快照。
 * 5. 并发不变量：pending_tasks_ 表示"已提交但未收尾"任务数。
 */
class ThreadPool final
{
public:
    class CancellationToken;
    class CancellationSource;

    explicit ThreadPool(const ThreadPoolConfig& config);
    explicit ThreadPool(std::size_t thread_count = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&)                    = delete;
    auto operator=(const ThreadPool&) -> ThreadPool& = delete;
    ThreadPool(ThreadPool&&)                         = delete;
    auto operator=(ThreadPool&&) -> ThreadPool&      = delete;

    // -------------------------------------------------------------------------
    // 提交接口
    // -------------------------------------------------------------------------

    /**
     * @brief 提交可返回结果的异步任务。
     * @return 与任务绑定的 future。
     * @throws std::runtime_error 入队失败。
     */
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>;

    /**
     * @brief 提交 fire-and-forget 任务。
     * @throws std::runtime_error 入队失败。
     */
    template <typename F, typename... Args>
    void post(F&& f, Args&&... args);

    /**
     * @brief 尝试提交 fire-and-forget 任务，不抛异常。
     * @return true 表示已提交；false 表示被拒绝。
     */
    template <typename F, typename... Args>
    [[nodiscard]] auto try_post(F&& f, Args&&... args) -> bool;

    /**
     * @brief 尝试提交有返回值的任务，不抛异常。
     * @return 成功返回 future；失败返回 nullopt。
     */
    template <typename F, typename... Args>
    [[nodiscard]] auto try_submit(F&& f, Args&&... args) -> std::optional<std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>>;

    // -------------------------------------------------------------------------
    // 取消接口
    // -------------------------------------------------------------------------

    /**
     * @brief 提交支持协作取消的任务。
     *
     * 若回调签名包含 CancellationToken，线程池会将 token 作为首参传入。
     */
    template <typename F, typename... Args>
    auto submit_cancellable(CancellationToken token, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, CancellationToken, std::decay_t<Args>...>>;

    template <typename F, typename... Args>
    [[nodiscard]] auto try_post_cancellable(CancellationToken token, F&& f, Args&&... args) -> bool;

    [[nodiscard]] static auto make_cancellation_source() -> CancellationSource;

    // -------------------------------------------------------------------------
    // 等待与关闭
    // -------------------------------------------------------------------------

    void               wait_for_tasks();
    [[nodiscard]] auto wait_for_tasks_for(std::chrono::milliseconds timeout) -> bool;
    [[nodiscard]] auto wait_for_tasks_until(std::chrono::steady_clock::time_point deadline) -> bool;

    void shutdown();
    void shutdown(ShutdownMode mode);

    // -------------------------------------------------------------------------
    // 查询
    // -------------------------------------------------------------------------

    [[nodiscard]] static auto is_worker_thread() noexcept -> bool;
    [[nodiscard]] static auto current_worker_index() noexcept -> std::size_t;

    [[nodiscard]] auto thread_count() const noexcept -> std::size_t;
    [[nodiscard]] auto pending_tasks() const noexcept -> std::size_t;
    [[nodiscard]] auto stats() const noexcept -> ThreadPoolStats;

private:
    // -------------------------------------------------------------------------
    // 任务类型别名
    // -------------------------------------------------------------------------

    using MoveOnlyFunction = bee::MoveOnlyFunction<void()>;

    // -------------------------------------------------------------------------
    // 常量
    // -------------------------------------------------------------------------

    static constexpr std::size_t kDefaultLocalQueueCapacity   = 256;
    static constexpr std::size_t kDefaultGlobalQueueCapacity  = 4096;
    static constexpr std::size_t kVictimSampleCount           = 4;
    static constexpr std::size_t kGlobalProbeCount            = 4;
    static constexpr std::size_t kMaxAdaptiveGlobalProbeCount = 16;

public:
    // -------------------------------------------------------------------------
    // CancellationToken / CancellationSource
    // -------------------------------------------------------------------------

    class CancellationToken
    {
    public:
        CancellationToken() = default;

        [[nodiscard]] auto stop_requested() const noexcept -> bool
        {
            return state_ && state_->load(std::memory_order_acquire);
        }

    private:
        explicit CancellationToken(const std::shared_ptr<std::atomic_bool>& state)
            : state_(state)
        {
        }

        std::shared_ptr<std::atomic_bool> state_;
        friend class CancellationSource;
    };

    class CancellationSource
    {
    public:
        CancellationSource()
            : state_(std::make_shared<std::atomic_bool>(false))
        {
        }

        [[nodiscard]] auto token() const noexcept -> CancellationToken
        {
            return CancellationToken(state_);
        }

        void request_stop() noexcept
        {
            state_->store(true, std::memory_order_release);
        }

    private:
        std::shared_ptr<std::atomic_bool> state_;
    };

private:
    // -------------------------------------------------------------------------
    // 内部方法
    // -------------------------------------------------------------------------

    [[nodiscard]] auto enqueue_task(MoveOnlyFunction&& task) -> bool;
    [[nodiscard]] auto pick_global_queue_for_submit() noexcept -> std::size_t;
    [[nodiscard]] auto try_push_to_global_queues(MoveOnlyFunction& task) -> bool;
    [[nodiscard]] auto try_pop_from_global_queues(std::size_t self_index, MoveOnlyFunction& task) -> bool;

    void notify_pending_zero() noexcept;
    void rollback_failed_submission() noexcept;
    void finalize_task(bool dropped) noexcept;
    void execute_task(MoveOnlyFunction& task) noexcept;

    [[nodiscard]] auto try_take_task(std::size_t self_index, MoveOnlyFunction& task, std::uint64_t& rng_state) -> bool;
    void               worker_loop(std::size_t worker_index, std::stop_token st);

    static auto make_cancelled_error() -> std::runtime_error;

    // -------------------------------------------------------------------------
    // 数据成员 — 按缓存行亲和性分组
    // -------------------------------------------------------------------------

    using LocalQueue  = ChaseLevDeque<MoveOnlyFunction>;
    using GlobalQueue = MPMCQueueBase<MoveOnlyFunction>;

    ThreadPoolConfig                          config_{};
    std::size_t                               worker_count_{0};
    std::vector<std::jthread>                 workers_;
    std::vector<std::unique_ptr<LocalQueue>>  local_queues_;
    std::vector<std::unique_ptr<GlobalQueue>> global_queues_;

    // 提交游标（外部线程热点）
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<std::size_t> submit_cursor_{0};

    // 窃取/全局探测游标（worker 热点）
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<std::size_t> steal_cursor_{0};
    std::atomic<std::size_t>   global_probe_cursor_{0};
    std::atomic<std::uint32_t> global_probe_budget_{kGlobalProbeCount};
    std::atomic<std::uint32_t> global_miss_streak_{0};
    std::atomic<std::size_t>   sleeping_workers_{0};

    // 信号量（独立缓存行）
    alignas(BEE_CACHE_LINE_SIZE) std::counting_semaphore<(std::numeric_limits<std::ptrdiff_t>::max)()> task_signal_{0};

    // 生命周期控制标志
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<bool> accepting_tasks_{true};
    std::atomic<bool>           drop_queued_tasks_{false};
    std::atomic<bool>           shutdown_force_zeroed_{false};
    std::atomic<LifecyclePhase> lifecycle_phase_{LifecyclePhase::Running};

    // 全局未完成任务计数（独立缓存行，多线程高频写）
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<std::size_t> pending_tasks_{0};
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<std::size_t> active_tasks_{0};

    // 统计计数器
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<std::uint64_t> submitted_tasks_{0};
    std::atomic<std::uint64_t> finalized_tasks_{0};
    std::atomic<std::uint64_t> dropped_tasks_{0};
    std::atomic<std::uint64_t> local_pop_hits_{0};
    std::atomic<std::uint64_t> global_pop_hits_{0};
    std::atomic<std::uint64_t> steal_hits_{0};
    std::atomic<std::uint64_t> steal_misses_{0};
    std::atomic<std::uint64_t> local_queue_overflow_fallbacks_{0};
    std::atomic<std::uint64_t> idle_wait_count_{0};

    // 等待者条件变量
    mutable std::mutex      wait_mutex_;
    std::condition_variable wait_cv_;

    // 线程本地信息
    static thread_local ThreadPool* tls_current_pool_;
    static thread_local std::size_t tls_worker_index_;
};

// =============================================================================
// Template API Implementation
// =============================================================================

template <typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
{
    using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

    auto packaged =
        std::packaged_task<ReturnType()>([fn = std::forward<F>(f), tup = std::make_tuple(std::forward<Args>(args)...)]() mutable -> ReturnType {
            return std::apply(std::move(fn), std::move(tup));
        });

    auto future = packaged.get_future();

    pending_tasks_.fetch_add(1, std::memory_order_acq_rel);
    submitted_tasks_.fetch_add(1, std::memory_order_relaxed);
    try {
        if (!enqueue_task(MoveOnlyFunction([task = std::move(packaged)]() mutable { task(); }))) {
            throw std::runtime_error("ThreadPool enqueue failed.");
        }
    } catch (...) {
        rollback_failed_submission();
        throw;
    }

    return future;
}

template <typename F, typename... Args>
void ThreadPool::post(F&& f, Args&&... args)
{
    auto fire_and_forget = [fn = std::forward<F>(f), tup = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        std::apply(std::move(fn), std::move(tup));
    };

    pending_tasks_.fetch_add(1, std::memory_order_acq_rel);
    submitted_tasks_.fetch_add(1, std::memory_order_relaxed);
    try {
        if (!enqueue_task(MoveOnlyFunction([task = std::move(fire_and_forget)]() mutable {
                try {
                    task();
                } catch (...) {
                }
            }))) {
            throw std::runtime_error("ThreadPool enqueue failed.");
        }
    } catch (...) {
        rollback_failed_submission();
        throw;
    }
}

template <typename F, typename... Args>
auto ThreadPool::try_post(F&& f, Args&&... args) -> bool
{
    auto fire_and_forget = [fn = std::forward<F>(f), tup = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        std::apply(std::move(fn), std::move(tup));
    };

    pending_tasks_.fetch_add(1, std::memory_order_acq_rel);
    submitted_tasks_.fetch_add(1, std::memory_order_relaxed);

    try {
        if (!enqueue_task(MoveOnlyFunction([task = std::move(fire_and_forget)]() mutable {
                try {
                    task();
                } catch (...) {
                }
            }))) {
            rollback_failed_submission();
            return false;
        }
    } catch (...) {
        rollback_failed_submission();
        return false;
    }

    return true;
}

template <typename F, typename... Args>
auto ThreadPool::try_submit(F&& f, Args&&... args) -> std::optional<std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>>
{
    using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

    std::optional<std::packaged_task<ReturnType()>> packaged;
    std::optional<std::future<ReturnType>>          future;

    try {
        packaged.emplace(
            std::packaged_task<ReturnType()>([fn = std::forward<F>(f), tup = std::make_tuple(std::forward<Args>(args)...)]() mutable -> ReturnType {
                return std::apply(std::move(fn), std::move(tup));
            })
        );
        future = packaged->get_future();
    } catch (...) {
        return std::nullopt;
    }

    pending_tasks_.fetch_add(1, std::memory_order_acq_rel);
    submitted_tasks_.fetch_add(1, std::memory_order_relaxed);

    try {
        if (!enqueue_task(MoveOnlyFunction([task = std::move(*packaged)]() mutable { task(); }))) {
            rollback_failed_submission();
            return std::nullopt;
        }
    } catch (...) {
        rollback_failed_submission();
        return std::nullopt;
    }

    return std::optional<std::future<ReturnType>>(std::move(*future));
}

template <typename F, typename... Args>
auto ThreadPool::submit_cancellable(CancellationToken token, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<F>, CancellationToken, std::decay_t<Args>...>>
{
    using ReturnType = std::invoke_result_t<std::decay_t<F>, CancellationToken, std::decay_t<Args>...>;

    auto packaged = std::packaged_task<ReturnType()>(
        [token, fn = std::forward<F>(f), tup = std::make_tuple(std::forward<Args>(args)...)]() mutable -> ReturnType {
            if (token.stop_requested()) {
                throw make_cancelled_error();
            }

            return std::apply(
                [&](auto&&... unpacked) -> ReturnType {
                    if constexpr (std::is_invocable_v<std::decay_t<F>&, CancellationToken, decltype(unpacked)...>) {
                        return fn(token, std::forward<decltype(unpacked)>(unpacked)...);
                    } else {
                        return fn(std::forward<decltype(unpacked)>(unpacked)...);
                    }
                },
                std::move(tup)
            );
        }
    );

    auto future = packaged.get_future();

    pending_tasks_.fetch_add(1, std::memory_order_acq_rel);
    submitted_tasks_.fetch_add(1, std::memory_order_relaxed);

    try {
        if (!enqueue_task(MoveOnlyFunction([task = std::move(packaged)]() mutable { task(); }))) {
            throw std::runtime_error("ThreadPool enqueue failed.");
        }
    } catch (...) {
        rollback_failed_submission();
        throw;
    }

    return future;
}

template <typename F, typename... Args>
auto ThreadPool::try_post_cancellable(CancellationToken token, F&& f, Args&&... args) -> bool
{
    auto work = [token, fn = std::forward<F>(f), tup = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        if (token.stop_requested()) {
            return;
        }

        std::apply(
            [&](auto&&... unpacked) {
                if constexpr (std::is_invocable_v<std::decay_t<F>&, CancellationToken, decltype(unpacked)...>) {
                    fn(token, std::forward<decltype(unpacked)>(unpacked)...);
                } else {
                    fn(std::forward<decltype(unpacked)>(unpacked)...);
                }
            },
            std::move(tup)
        );
    };

    return try_post(std::move(work));
}

inline auto ThreadPool::make_cancellation_source() -> CancellationSource
{
    return CancellationSource();
}

inline auto ThreadPool::is_worker_thread() noexcept -> bool
{
    return tls_current_pool_ != nullptr;
}

inline auto ThreadPool::current_worker_index() noexcept -> std::size_t
{
    return tls_worker_index_;
}

} // namespace bee
