/**
 * @File WorkPool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/17
 * @Brief This file is part of Bee.
 */

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "Base/Core/Defines.hpp"
#include "Base/Core/MoveOnlyFunction.hpp"
#include "Base/Sync/EventCount.hpp"
#include "Concurrency/LockFree/ChaseLevDeque.hpp"
#include "Concurrency/LockFree/MPMCQueue.hpp"
#include "Concurrency/Threading.hpp"

namespace bee
{

// =============================================================================
// 枚举与配置
// =============================================================================

/// 任务优先级：High 最先被消费，Low 最后
enum class TaskPriority : u8
{
    Low    = 0,
    Normal = 1,
    High   = 2,
};

/// 关停模式
enum class ShutdownMode
{
    Drain,     ///< 等待所有已入队任务执行完毕后再停止
    Immediate, ///< 立即停止，丢弃尚未开始执行的任务
};

/// 工作池配置
struct WorkPoolConfig
{
    std::size_t thread_count          = std::thread::hardware_concurrency();
    std::size_t local_queue_capacity  = 256;
    std::size_t global_queue_capacity = 4096;
};

/// 运行时统计信息（快照，非精确一致）
struct WorkPoolStats
{
    std::size_t   worker_count{0};
    std::size_t   pending_tasks{0};
    std::uint64_t completed_tasks{0};
    std::uint64_t stolen_tasks{0};
};

// =============================================================================
// WorkPool — 基于工作窃取的线程池
//
// 调度架构：
//   · 每个优先级各一个全局 MPMC 队列（High / Normal / Low）
//   · 每个工作线程拥有一个本地 Chase-Lev Deque
//   · 任务获取顺序：本地出队 → 全局 High → Normal → Low → 窃取
//   · 唤醒机制：无条件信号量 acquire，杜绝 TOCTOU 竞态
// =============================================================================

class WorkPool final
{
public:
    // ── 构造与析构 ──

    explicit WorkPool(const WorkPoolConfig& cfg = {});
    explicit WorkPool(std::size_t thread_count);
    ~WorkPool();

    WorkPool(const WorkPool&)                    = delete;
    WorkPool(WorkPool&&)                         = delete;
    auto operator=(const WorkPool&) -> WorkPool& = delete;
    auto operator=(WorkPool&&) -> WorkPool&      = delete;

    // ── 提交任务 ──

    /// 提交任务（fire-and-forget），入队失败时抛出异常
    template <typename F>
    void post(F&& f, TaskPriority pri = TaskPriority::Normal);

    /// 尝试提交任务，队列满或已关停时返回 false，不抛异常
    template <typename F>
    [[nodiscard]] auto try_post(F&& f, TaskPriority pri = TaskPriority::Normal) -> bool;

    /// 提交任务并获取 future，入队失败时抛出异常
    template <typename F>
    [[nodiscard]] auto submit(F&& f, TaskPriority pri = TaskPriority::Normal) -> std::future<std::invoke_result_t<F>>;

    // ── 等待与关停 ──

    /// 阻塞等待，直到所有已入队任务执行完毕
    void wait_for_tasks();

    /// 带超时的等待，返回是否在超时前完成
    [[nodiscard]] auto wait_for_tasks_for(std::chrono::milliseconds timeout) -> bool;

    /// 关停工作池；Drain 模式会执行完剩余任务，Immediate 模式丢弃未开始的任务
    void shutdown(ShutdownMode mode = ShutdownMode::Drain);

    // ── Scheduler concept 适配 ──

    /// co_await pool.schedule() 将执行转移到工作线程
    struct ScheduleAwaiter
    {
        WorkPool* pool;

        [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
        auto await_suspend(std::coroutine_handle<> h) -> void;
        auto await_resume() const noexcept -> void {}
    };

    /// 返回一个 awaiter，co_await 后将执行转移到工作线程。
    /// 这是 P2300 schedule(scheduler) 的协程等价物。
    auto schedule() -> ScheduleAwaiter { return ScheduleAwaiter{this}; }

    // ── 查询 ──

    /// 工作线程数
    [[nodiscard]] auto thread_count() const noexcept -> std::size_t;

    /// 当前待处理任务数
    [[nodiscard]] auto pending_tasks() const noexcept -> std::size_t;

    /// 运行时统计快照
    [[nodiscard]] auto stats() const noexcept -> WorkPoolStats;

    /// 当前线程是否是本工作池的工作线程
    [[nodiscard]] static auto is_worker_thread() noexcept -> bool;

private:
    // ── 内部类型别名 ──
    using Task        = MoveOnlyFunction<void()>;
    using LocalQueue  = ChaseLevDeque<Task>;
    using GlobalQueue = MPMCQueue<Task>;

    // ── 常量 ──
    static constexpr std::size_t   kMaxStealAttempts = 4;  ///< 每轮窃取尝试上限
    static constexpr std::uint32_t kSpinLimit        = 32; ///< 纯 pause 自旋上限
    static constexpr std::uint32_t kYieldLimit       = 64; ///< 含 yield 的自旋上限
    static constexpr std::size_t   kPriorityCount    = 3;  ///< 优先级数量

    // ── 内部方法（定义在 .cpp 中） ──

    /// 将任务入队到合适的队列中（本地 / 全局），成功时增加 pending 并唤醒工作线程
    [[nodiscard]] auto enqueue_task(Task&& task, TaskPriority pri) -> bool;

    /// 任务执行完毕（或被丢弃）后的清理：更新计数器，唤醒等待者
    void finalize_task(bool dropped) noexcept;

    /// 执行一个任务；若处于 Immediate 关停模式则直接丢弃
    void execute_task(Task& task) noexcept;

    /// 尝试从本地队列、全局队列、其他工作线程获取一个任务
    [[nodiscard]] auto try_take_task(std::size_t self, Task& task, std::uint64_t& rng) -> bool;

    /// 从随机受害者的本地队列窃取一个任务
    [[nodiscard]] auto try_steal(std::size_t self, Task& task, std::uint64_t& rng) -> bool;

    /// 工作线程主循环
    void worker_loop(std::size_t index, std::stop_token st);

    /// 关停阶段：排空剩余可获取的任务
    void drain_remaining(std::size_t index);

    /// pending 计数归零时唤醒所有 wait_for_tasks 等待者
    void notify_pending_zero() noexcept;

    /// 优先级 → 全局队列下标映射（High=0, Normal=1, Low=2，与扫描顺序一致）
    static auto priority_to_index(TaskPriority pri) noexcept -> std::size_t
    {
        return static_cast<std::size_t>(TaskPriority::High) - static_cast<std::size_t>(pri);
    }

    // ── 数据成员 ──

    // 只读区：构造后不再修改
    WorkPoolConfig config_{};
    std::size_t    worker_count_{0};

    // 队列
    std::vector<std::unique_ptr<LocalQueue>>                 local_queues_;
    std::array<std::unique_ptr<GlobalQueue>, kPriorityCount> global_queues_;

    // 工作线程（jthread 析构时自动 request_stop + join）
    std::vector<std::jthread> workers_;

    // 生命周期标志（低频访问，同一缓存行）
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<bool> accepting_{true};
    std::atomic<bool> draining_{false};

    // 工作线程空闲唤醒（高频访问，独占缓存行）。
    // 使用 EventCount 替代 counting_semaphore：
    //   · 统一 Bee 的 Task 同步原语（get/wait 也走 EventCount）；
    //   · 无等待者时 notify 零 syscall 代价；
    //   · 支持 notify_all 精确唤醒所有 waiter（关停场景）。
    // 生产者：入队成功后 notify()；消费者：标准 prepare/wait 协议。
    alignas(BEE_CACHE_LINE_SIZE) EventCount idle_ec_;

    // 计数器（各占独立缓存行，避免伪共享）
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<std::size_t> pending_tasks_{0};
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<std::uint64_t> completed_tasks_{0};
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<std::uint64_t> stolen_tasks_{0};

    // 等待机制
    mutable std::mutex      wait_mutex_;
    std::condition_variable wait_cv_;

    // 线程局部存储（标识当前线程所属的工作池和工作线程编号）
    static thread_local WorkPool*   tls_pool_;
    static thread_local std::size_t tls_worker_index_;
};

// =============================================================================
// 模板方法实现
// =============================================================================

template <typename F>
void WorkPool::post(F&& f, TaskPriority pri)
{
    // 包装用户可调用对象，吞掉异常（fire-and-forget 语义）
    Task task([fn = std::forward<F>(f)]() mutable {
        try {
            fn();
        } catch (...) {
        }
    });

    if (!enqueue_task(std::move(task), pri)) {
        throw std::runtime_error("WorkPool: enqueue failed (pool shut down or queue full).");
    }
}

template <typename F>
auto WorkPool::try_post(F&& f, TaskPriority pri) -> bool
{
    // 快速路径：已关停则直接返回，避免不必要的 MoveOnlyFunction 构造
    if (!accepting_.load(std::memory_order_relaxed)) {
        return false;
    }

    Task task([fn = std::forward<F>(f)]() mutable {
        try {
            fn();
        } catch (...) {
        }
    });

    return enqueue_task(std::move(task), pri);
}

template <typename F>
auto WorkPool::submit(F&& f, TaskPriority pri) -> std::future<std::invoke_result_t<F>>
{
    using R = std::invoke_result_t<F>;

    // packaged_task 会自动捕获异常到 promise 中，无需额外 try/catch
    auto packaged = std::packaged_task<R()>(std::forward<F>(f));
    auto future   = packaged.get_future();

    Task task([pt = std::move(packaged)]() mutable { pt(); });

    if (!enqueue_task(std::move(task), pri)) {
        throw std::runtime_error("WorkPool: enqueue failed (pool shut down or queue full).");
    }

    return future;
}

inline auto WorkPool::ScheduleAwaiter::await_suspend(std::coroutine_handle<> h) -> void
{
    pool->post([h]() mutable { h.resume(); });
}

inline auto WorkPool::is_worker_thread() noexcept -> bool
{
    return tls_pool_ != nullptr;
}

} // namespace bee
