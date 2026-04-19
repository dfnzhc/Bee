/**
 * @File WorkPool.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/17
 * @Brief This file is part of Bee.
 */

#include "Thread/WorkPool.hpp"

#include <algorithm>
#include <format>

namespace bee
{

// =============================================================================
// 线程局部存储
// =============================================================================

thread_local WorkPool*   WorkPool::tls_pool_         = nullptr;
thread_local std::size_t WorkPool::tls_worker_index_ = static_cast<std::size_t>(-1);

// =============================================================================
// 构造与析构
// =============================================================================

WorkPool::WorkPool(std::size_t thread_count)
    : WorkPool(WorkPoolConfig{thread_count})
{
}

WorkPool::WorkPool(const WorkPoolConfig& cfg)
    : config_(cfg)
{
    // 至少保证 1 个工作线程
    worker_count_ = std::max<std::size_t>(cfg.thread_count, 1);

    // 创建三个全局优先级队列（High=0, Normal=1, Low=2）
    for (std::size_t i = 0; i < kPriorityCount; ++i) {
        global_queues_[i] = std::make_unique<GlobalQueue>(config_.global_queue_capacity);
    }

    // 创建每个工作线程的本地 Chase-Lev Deque
    local_queues_.reserve(worker_count_);
    for (std::size_t i = 0; i < worker_count_; ++i) {
        local_queues_.emplace_back(std::make_unique<LocalQueue>(config_.local_queue_capacity));
    }

    // 启动工作线程（jthread 自带 stop_token）
    workers_.reserve(worker_count_);
    for (std::size_t i = 0; i < worker_count_; ++i) {
        workers_.emplace_back([this, i](std::stop_token st) { worker_loop(i, std::move(st)); });
    }
}

WorkPool::~WorkPool()
{
    // 析构时若尚未关停，则以 Drain 模式关停
    if (accepting_.load(std::memory_order_relaxed)) {
        shutdown(ShutdownMode::Drain);
    }
}

// =============================================================================
// 关停
// =============================================================================

void WorkPool::shutdown(ShutdownMode mode)
{
    // 幂等：仅第一次调用生效
    bool expected = true;
    if (!accepting_.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) {
        // 已经关停过 —— 若从外部线程调用，等待工作线程退出
        if (tls_pool_ != this) {
            workers_.clear(); // jthread 析构自动 request_stop + join
        }
        return;
    }

    // Immediate 模式：标记 draining，后续 execute_task 会丢弃未执行的任务
    if (mode == ShutdownMode::Immediate) {
        draining_.store(true, std::memory_order_release);
    }

    // 请求所有工作线程停止，并唤醒它们（notify_all 确保睡眠中的线程能醒来）
    for (auto& w : workers_) {
        w.request_stop();
    }
    idle_ec_.notify_all();

    // 若从工作线程内部发起关停（如任务回调中），不能自 join
    if (tls_pool_ == this) {
        return;
    }

    // jthread 析构自动 join；工作线程在退出前已各自调用了 drain_remaining
    workers_.clear();

    // 补充安全网：工作线程全部退出后，可能仍有极少量任务因 TOCTOU 窗口
    // 留在队列中（enqueue_task 在 accepting_ 置 false 之前通过了检查，
    // 但在工作线程排空后才完成 push）。此处做最终一轮清扫。
    Task task;
    for (std::size_t i = 0; i < worker_count_; ++i) {
        while (local_queues_[i]->try_pop(task)) {
            execute_task(task);
        }
    }
    for (std::size_t i = 0; i < kPriorityCount; ++i) {
        while (global_queues_[i]->try_pop(task)) {
            execute_task(task);
        }
    }
}

// =============================================================================
// 等待
// =============================================================================

void WorkPool::wait_for_tasks()
{
    std::unique_lock lock(wait_mutex_);
    wait_cv_.wait(lock, [this] { return pending_tasks_.load(std::memory_order_acquire) == 0; });
}

auto WorkPool::wait_for_tasks_for(std::chrono::milliseconds timeout) -> bool
{
    std::unique_lock lock(wait_mutex_);
    return wait_cv_.wait_for(lock, timeout, [this] { return pending_tasks_.load(std::memory_order_acquire) == 0; });
}

// =============================================================================
// 查询
// =============================================================================

auto WorkPool::thread_count() const noexcept -> std::size_t
{
    return worker_count_;
}

auto WorkPool::pending_tasks() const noexcept -> std::size_t
{
    return pending_tasks_.load(std::memory_order_acquire);
}

auto WorkPool::stats() const noexcept -> WorkPoolStats
{
    WorkPoolStats s;
    s.worker_count    = worker_count_;
    s.pending_tasks   = pending_tasks_.load(std::memory_order_relaxed);
    s.completed_tasks = completed_tasks_.load(std::memory_order_relaxed);
    s.stolen_tasks    = stolen_tasks_.load(std::memory_order_relaxed);
    return s;
}

// =============================================================================
// 内部辅助
// =============================================================================

void WorkPool::notify_pending_zero() noexcept
{
    std::lock_guard lock(wait_mutex_);
    wait_cv_.notify_all();
}

void WorkPool::finalize_task(bool dropped) noexcept
{
    // 任务完成：增加已完成计数（被丢弃的任务不计入）
    if (!dropped) {
        completed_tasks_.fetch_add(1, std::memory_order_relaxed);
    }

    // pending 减一；若归零则唤醒 wait_for_tasks 的等待者
    if (pending_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        notify_pending_zero();
    }
}

// =============================================================================
// 核心调度
// =============================================================================

auto WorkPool::enqueue_task(Task&& task, TaskPriority pri) -> bool
{
    if (!accepting_.load(std::memory_order_acquire)) {
        return false;
    }

    // 单工作线程 + 从该线程内部提交：直接内联执行，防止 submit().get() 自死锁
    if (tls_pool_ == this && worker_count_ == 1) {
        pending_tasks_.fetch_add(1, std::memory_order_acq_rel);
        execute_task(task);
        return true;
    }

    // 关键原则：先增加 pending 计数，再入队。
    // 若先入队后计数，则另一个线程可能在 fetch_add 之前就 steal + execute + finalize，
    // 导致 pending 从 0 减到 SIZE_MAX，notify_pending_zero() 永远不被触发，
    // wait_for_tasks() 永久阻塞。
    // 入队失败时回滚计数。

    pending_tasks_.fetch_add(1, std::memory_order_acq_rel);

    // 工作线程内部提交：优先推入本地 deque（忽略优先级，换取缓存局部性）
    if (tls_pool_ == this && tls_worker_index_ < local_queues_.size()) {
        if (local_queues_[tls_worker_index_]->try_push(std::move(task))) {
            idle_ec_.notify();
            return true;
        }
        // 本地队列已满，降级到全局队列
    }

    // 推入对应优先级的全局 MPMC 队列
    const std::size_t idx = priority_to_index(pri);
    if (global_queues_[idx]->try_push(std::move(task))) {
        idle_ec_.notify();
        return true;
    }

    // 所有队列均已满，回滚 pending 计数
    if (pending_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        notify_pending_zero();
    }
    return false;
}

void WorkPool::execute_task(Task& task) noexcept
{
    // Immediate 关停模式：丢弃尚未执行的任务
    if (draining_.load(std::memory_order_acquire)) {
        finalize_task(/*dropped=*/true);
        return;
    }

    // 防御性检查：若任务为空（极端竞态或逻辑错误），视为丢弃
    if (!task) {
        finalize_task(/*dropped=*/true);
        return;
    }

    try {
        task();
    } catch (...) {
        // post 已用 try/catch 包裹；submit 通过 packaged_task 捕获到 promise。
        // 此处是最终安全网，防止未捕获异常逃逸到工作线程。
    }
    finalize_task(/*dropped=*/false);
}

auto WorkPool::try_take_task(std::size_t self, Task& task, std::uint64_t& rng) -> bool
{
    // 1) 本地 deque LIFO 弹出（缓存亲和性最佳）
    if (local_queues_[self]->try_pop(task)) {
        return true;
    }

    // 2) 全局队列按优先级扫描：High → Normal → Low
    for (std::size_t i = 0; i < kPriorityCount; ++i) {
        if (global_queues_[i]->try_pop(task)) {
            return true;
        }
    }

    // 3) 从随机受害者窃取
    return try_steal(self, task, rng);
}

auto WorkPool::try_steal(std::size_t self, Task& task, std::uint64_t& rng) -> bool
{
    const std::size_t n = worker_count_;
    if (n <= 1) {
        return false;
    }

    // 最多尝试 min(N-1, kMaxStealAttempts) 个随机受害者
    const std::size_t attempts = std::min(n - 1, kMaxStealAttempts);
    for (std::size_t i = 0; i < attempts; ++i) {
        // xorshift64 伪随机数生成
        rng ^= (rng << 13);
        rng ^= (rng >> 7);
        rng ^= (rng << 17);

        // 映射到 [0, n-1) 范围并跳过自身
        std::size_t victim = static_cast<std::size_t>(rng % (n - 1));
        if (victim >= self) {
            ++victim;
        }

        if (local_queues_[victim]->try_steal(task)) {
            stolen_tasks_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }

    return false;
}

// =============================================================================
// 工作线程主循环
// =============================================================================

void WorkPool::worker_loop(std::size_t index, std::stop_token st)
{
    set_current_thread_name(std::format("bee-work-{}", index));

    tls_pool_         = this;
    tls_worker_index_ = index;

    // 以 Knuth 乘法哈希为每个工作线程生成不同的 RNG 种子
    std::uint64_t rng = (static_cast<std::uint64_t>(index) + 1u) * 0x9e3779b97f4a7c15ULL;

    while (!st.stop_requested()) {
        Task task;

        // 快速路径：立即尝试获取任务
        if (try_take_task(index, task, rng)) {
            execute_task(task);
            continue;
        }

        // 自适应自旋：pause → yield → 阻塞
        std::uint32_t spin     = 0;
        bool          got_task = false;
        while (spin < kYieldLimit) {
            adaptive_backoff(spin, kSpinLimit, kYieldLimit);
            if (try_take_task(index, task, rng)) {
                got_task = true;
                break;
            }
        }

        if (got_task) {
            execute_task(task);
            continue;
        }

        // 所有自旋耗尽，阻塞在 EventCount 上。
        // 严格遵循 check-prepare-check-wait：先登记等待再次检查任务/停止
        // 信号，否则会与生产者 enqueue→notify 的时序产生 TOCTOU 竞态。
        auto key = idle_ec_.prepare_wait();
        if (st.stop_requested()) {
            idle_ec_.cancel_wait();
            break;
        }
        if (try_take_task(index, task, rng)) {
            idle_ec_.cancel_wait();
            execute_task(task);
            continue;
        }
        idle_ec_.wait(key);
    }

    // stop 信号已收到：排空剩余可获取的任务
    drain_remaining(index);

    tls_pool_         = nullptr;
    tls_worker_index_ = static_cast<std::size_t>(-1);
}

void WorkPool::drain_remaining(std::size_t index)
{
    // 用不同种子，避免多个工作线程同时窃取同一受害者
    std::uint64_t rng = (static_cast<std::uint64_t>(index) + 1u) * 0x517cc1b727220a95ULL;
    Task          task;

    while (try_take_task(index, task, rng)) {
        execute_task(task);
    }
}

} // namespace bee
