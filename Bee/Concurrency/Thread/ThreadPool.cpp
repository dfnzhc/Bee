/**
 * @File ThreadPool.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#include "Thread/ThreadPool.hpp"

#include <algorithm>
#include <format>
#include <limits>

namespace bee
{

thread_local ThreadPool* ThreadPool::tls_current_pool_ = nullptr;
thread_local std::size_t ThreadPool::tls_worker_index_ = static_cast<std::size_t>(-1);

ThreadPool::ThreadPool(std::size_t thread_count)
    : ThreadPool(ThreadPoolConfig{thread_count, BackpressurePolicy::FailFast, std::chrono::milliseconds(50)})
{
}

ThreadPool::ThreadPool(const ThreadPoolConfig& config)
    : config_(config)
{
    worker_count_ = std::max<std::size_t>(config_.thread_count, 1);

    const std::size_t shard_count = std::max<std::size_t>(1, std::min<std::size_t>(worker_count_, 8));
    global_queues_.reserve(shard_count);
    for (std::size_t i = 0; i < shard_count; ++i) {
        global_queues_.emplace_back(std::make_unique<GlobalQueue>(kDefaultGlobalQueueCapacity));
    }

    local_queues_.reserve(worker_count_);
    for (std::size_t i = 0; i < worker_count_; ++i) {
        local_queues_.emplace_back(std::make_unique<LocalQueue>(kDefaultLocalQueueCapacity));
    }

    workers_.reserve(worker_count_);
    for (std::size_t i = 0; i < worker_count_; ++i) {
        workers_.emplace_back([this, i](std::stop_token st) {
            worker_loop(i, std::move(st));
        });
    }
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

void ThreadPool::wait_for_tasks()
{
    std::unique_lock lock(wait_mutex_);
    wait_cv_.wait(lock, [this] {
        return pending_tasks_.load(std::memory_order_acquire) == 0;
    });
}

auto ThreadPool::wait_for_tasks_for(std::chrono::milliseconds timeout) -> bool
{
    std::unique_lock lock(wait_mutex_);
    return wait_cv_.wait_for(lock, timeout, [this] {
        return pending_tasks_.load(std::memory_order_acquire) == 0;
    });
}

auto ThreadPool::wait_for_tasks_until(std::chrono::steady_clock::time_point deadline) -> bool
{
    std::unique_lock lock(wait_mutex_);
    return wait_cv_.wait_until(lock, deadline, [this] {
        return pending_tasks_.load(std::memory_order_acquire) == 0;
    });
}

void ThreadPool::shutdown()
{
    shutdown(ShutdownMode::Drain);
}

void ThreadPool::shutdown(ShutdownMode mode)
{
    LifecyclePhase expected_phase = LifecyclePhase::Running;
    if (!lifecycle_phase_.compare_exchange_strong(expected_phase, LifecyclePhase::Quiescing, std::memory_order_acq_rel)) {
        return;
    }
    accepting_tasks_.store(false, std::memory_order_release);

    if (mode == ShutdownMode::Immediate) {
        drop_queued_tasks_.store(true, std::memory_order_release);
        task_signal_.release(static_cast<std::ptrdiff_t>(workers_.size()));
    }

    lifecycle_phase_.store(LifecyclePhase::Draining, std::memory_order_release);

    // 有界等待：防止 Immediate 模式下残留 pending 导致永久阻塞。
    // 正常路径下 worker 会通过 finalize_task 递减 pending，此等待极少超时。
    if (!wait_for_tasks_until(std::chrono::steady_clock::now() + std::chrono::seconds(30))) {
        // 超时兜底：强制将残留 pending 归零。
        // 这只会在极端竞态下发生（如 pending 已递增但任务尚未入队）。
        shutdown_force_zeroed_.store(true, std::memory_order_release);
        const std::size_t stale = pending_tasks_.exchange(0, std::memory_order_acq_rel);
        if (stale > 0) {
            dropped_tasks_.fetch_add(stale, std::memory_order_relaxed);
            finalized_tasks_.fetch_add(stale, std::memory_order_relaxed);
            notify_pending_zero();
        }
    }

    lifecycle_phase_.store(LifecyclePhase::Stopping, std::memory_order_release);

    // 请求所有 worker 退出并唤醒阻塞的信号量。
    for (auto& w : workers_) {
        w.request_stop();
    }
    task_signal_.release(static_cast<std::ptrdiff_t>(workers_.size()));

    workers_.clear();
    lifecycle_phase_.store(LifecyclePhase::Stopped, std::memory_order_release);
}

auto ThreadPool::thread_count() const noexcept -> std::size_t
{
    return worker_count_;
}

auto ThreadPool::pending_tasks() const noexcept -> std::size_t
{
    return pending_tasks_.load(std::memory_order_acquire);
}

auto ThreadPool::stats() const noexcept -> ThreadPoolStats
{
    ThreadPoolStats s;
    s.worker_count                   = worker_count_;
    s.phase                          = lifecycle_phase_.load(std::memory_order_relaxed);
    s.pending_tasks                  = pending_tasks_.load(std::memory_order_relaxed);
    s.active_tasks                   = active_tasks_.load(std::memory_order_relaxed);
    s.submitted_tasks                = submitted_tasks_.load(std::memory_order_relaxed);
    s.finalized_tasks                = finalized_tasks_.load(std::memory_order_relaxed);
    s.dropped_tasks                  = dropped_tasks_.load(std::memory_order_relaxed);
    s.local_pop_hits                 = local_pop_hits_.load(std::memory_order_relaxed);
    s.global_pop_hits                = global_pop_hits_.load(std::memory_order_relaxed);
    s.steal_hits                     = steal_hits_.load(std::memory_order_relaxed);
    s.steal_misses                   = steal_misses_.load(std::memory_order_relaxed);
    s.local_queue_overflow_fallbacks = local_queue_overflow_fallbacks_.load(std::memory_order_relaxed);
    s.idle_wait_count                = idle_wait_count_.load(std::memory_order_relaxed);
    s.global_probe_budget            = global_probe_budget_.load(std::memory_order_relaxed);
    return s;
}

auto ThreadPool::enqueue_task(MoveOnlyFunction&& task) -> bool
{
    if (!accepting_tasks_.load(std::memory_order_acquire)) {
        return false;
    }

    // Chase-Lev push/pop 限 owner 线程；仅池内 worker 走本地队列。
    if (tls_current_pool_ == this && tls_worker_index_ < local_queues_.size()) {
        auto& local = *local_queues_[tls_worker_index_];
        if (local.size_approx() < static_cast<std::ptrdiff_t>(local.capacity())) {
            local.push(std::move(task));
            task_signal_.release();
            return true;
        }
        local_queue_overflow_fallbacks_.fetch_add(1, std::memory_order_relaxed);
    }

    if (!try_push_to_global_queues(task)) {
        if (config_.backpressure_policy == BackpressurePolicy::CallerRuns && task) {
            execute_task(task);
            return true;
        }
        return false;
    }

    task_signal_.release();
    return true;
}

auto ThreadPool::pick_global_queue_for_submit() noexcept -> std::size_t
{
    const std::size_t shard_count = global_queues_.size();
    if (shard_count == 1) {
        return 0;
    }

    const std::size_t tid_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    const std::size_t rr       = submit_cursor_.fetch_add(1, std::memory_order_relaxed);
    return (tid_hash ^ rr) % shard_count;
}

auto ThreadPool::try_push_to_global_queues(MoveOnlyFunction& task) -> bool
{
    if (!task) {
        return false;
    }

    const std::size_t shard_count = global_queues_.size();
    if (shard_count == 0) {
        return false;
    }

    const std::size_t start = pick_global_queue_for_submit();

    auto try_once = [&](std::size_t bias) -> bool {
        for (std::size_t i = 0; i < shard_count; ++i) {
            const std::size_t index = (start + bias + i) % shard_count;
            if (global_queues_[index]->try_push(std::move(task))) {
                return true;
            }
            if (!task) {
                return false;
            }
        }
        return false;
    };

    if (config_.backpressure_policy != BackpressurePolicy::Block) {
        // FailFast / CallerRuns: 各分片只尝试一轮，不做退避旋转。
        return try_once(0);
    }

    const auto block_deadline = std::chrono::steady_clock::now() + config_.enqueue_block_timeout;
    std::uint32_t spin        = 0;
    while (true) {
        if (try_once(spin)) {
            return true;
        }

        if (std::chrono::steady_clock::now() >= block_deadline) {
            return false;
        }

        bee::adaptive_backoff(spin, 64u, 512u);
    }
}

auto ThreadPool::try_pop_from_global_queues(std::size_t self_index, MoveOnlyFunction& task) -> bool
{
    const std::size_t shard_count = global_queues_.size();
    if (shard_count == 0) {
        return false;
    }

    const std::size_t home = self_index % shard_count;
    if (global_queues_[home]->try_pop(task)) {
        global_miss_streak_.store(0, std::memory_order_relaxed);
        const std::uint32_t budget = global_probe_budget_.load(std::memory_order_relaxed);
        if (budget > 1) {
            global_probe_budget_.store(budget - 1, std::memory_order_relaxed);
        }
        global_pop_hits_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    const std::uint32_t budget = std::max<std::uint32_t>(1u, global_probe_budget_.load(std::memory_order_relaxed));
    const std::size_t probes   = std::min<std::size_t>(shard_count - 1, budget);
    const std::size_t start    = global_probe_cursor_.fetch_add(1, std::memory_order_relaxed);
    for (std::size_t i = 1; i <= probes; ++i) {
        const std::size_t idx = (start + home + i) % shard_count;
        if (global_queues_[idx]->try_pop(task)) {
            global_miss_streak_.store(0, std::memory_order_relaxed);
            global_pop_hits_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }

    const std::uint32_t streak = global_miss_streak_.fetch_add(1, std::memory_order_relaxed) + 1;
    if ((streak & 0x1Fu) == 0x1Fu) {
        const std::uint32_t cur = global_probe_budget_.load(std::memory_order_relaxed);
        const std::uint32_t cap = static_cast<std::uint32_t>(
            std::min<std::size_t>(kMaxAdaptiveGlobalProbeCount, shard_count > 0 ? shard_count - 1 : 0));
        if (cap > 0 && cur < cap) {
            global_probe_budget_.store(cur + 1, std::memory_order_relaxed);
        }
    }

    return false;
}

void ThreadPool::notify_pending_zero() noexcept
{
    std::lock_guard<std::mutex> lock(wait_mutex_);
    wait_cv_.notify_all();
    task_signal_.release();
}

void ThreadPool::rollback_failed_submission() noexcept
{
    submitted_tasks_.fetch_sub(1, std::memory_order_acq_rel);

    const std::size_t remaining = pending_tasks_.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (remaining == 0) {
        notify_pending_zero();
    }
}

void ThreadPool::finalize_task(bool dropped) noexcept
{
    if (dropped) {
        dropped_tasks_.fetch_add(1, std::memory_order_relaxed);
    }

    finalized_tasks_.fetch_add(1, std::memory_order_relaxed);

    // shutdown force-zero 已将所有残留 pending 统一归零，跳过递减以避免下溢。
    if (shutdown_force_zeroed_.load(std::memory_order_acquire)) {
        return;
    }

    const std::size_t remaining = pending_tasks_.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (remaining == 0) {
        notify_pending_zero();
    }
}

auto ThreadPool::make_cancelled_error() -> std::runtime_error
{
    return std::runtime_error("Task cancelled");
}

auto ThreadPool::try_take_task(std::size_t self_index, MoveOnlyFunction& task, std::uint64_t& rng_state) -> bool
{
    // 1) 优先本地弹出，保持缓存局部性。
    if (local_queues_[self_index]->try_pop(task)) {
        local_pop_hits_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    // 2) 再尝试全局分片队列。
    if (try_pop_from_global_queues(self_index, task)) {
        return true;
    }

    // 3) 从其他本地队列窃取（xorshift64 随机采样 + 轮询回退）。
    const std::size_t total  = local_queues_.size();
    std::size_t best_victim  = self_index;
    std::ptrdiff_t best_load = std::numeric_limits<std::ptrdiff_t>::min();

    const std::size_t samples = std::min<std::size_t>(kVictimSampleCount, total > 1 ? total - 1 : 0);
    for (std::size_t s = 0; s < samples; ++s) {
        rng_state             ^= (rng_state << 12);
        rng_state             ^= (rng_state >> 25);
        rng_state             ^= (rng_state << 27);
        std::size_t candidate = static_cast<std::size_t>(rng_state % total);
        if (candidate == self_index) {
            candidate = (candidate + 1) % total;
        }

        const std::ptrdiff_t load = local_queues_[candidate]->size_approx();
        if (load > best_load) {
            best_load   = load;
            best_victim = candidate;
        }
    }

    if (best_victim != self_index && local_queues_[best_victim]->try_steal(task)) {
        steal_hits_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    // 轮询回退，提高偏斜负载下的公平性。
    const std::size_t start = steal_cursor_.fetch_add(1, std::memory_order_relaxed);
    for (std::size_t offset = 1; offset < total; ++offset) {
        const std::size_t victim = (start + self_index + offset) % total;
        if (victim == best_victim) {
            continue;
        }
        if (local_queues_[victim]->try_steal(task)) {
            steal_hits_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }

    steal_misses_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

void ThreadPool::execute_task(MoveOnlyFunction& task) noexcept
{
    if (drop_queued_tasks_.load(std::memory_order_acquire)) {
        finalize_task(true);
        return;
    }
    active_tasks_.fetch_add(1, std::memory_order_relaxed);
    try {
        task();
    } catch (...) {
        // fire-and-forget 语义下无 future 可回传异常。
        // submit 的 packaged_task 已由 promise 内部捕获异常。
    }
    active_tasks_.fetch_sub(1, std::memory_order_relaxed);
    finalize_task(false);
}

void ThreadPool::worker_loop(std::size_t worker_index, std::stop_token st)
{
    set_current_thread_name(std::format("bee-pool-{}", worker_index));

    tls_current_pool_ = this;
    tls_worker_index_ = worker_index;

    std::uint64_t rng_state = (static_cast<std::uint64_t>(worker_index) + 1u) * 0x9e3779b97f4a7c15ULL;

    while (true) {
        MoveOnlyFunction task;

        if (try_take_task(worker_index, task, rng_state)) {
            execute_task(task);
            continue;
        }

        if (st.stop_requested() && pending_tasks_.load(std::memory_order_acquire) == 0) {
            break;
        }

        // 自适应自旋：根据 backlog 调整忙等预算。
        bool acquired                   = false;
        const std::size_t pending       = pending_tasks_.load(std::memory_order_relaxed);
        const std::size_t active        = active_tasks_.load(std::memory_order_relaxed);
        const std::size_t backlog       = pending > active ? pending - active : 0;
        const std::uint32_t spin_budget = backlog > worker_count_ ? 256u : (backlog > 0 ? 128u : 32u);

        std::uint32_t spin = 0;
        while (spin < spin_budget) {
            if (try_take_task(worker_index, task, rng_state)) {
                acquired = true;
                break;
            }
            bee::adaptive_backoff(spin, 32u, 256u);
        }

        if (acquired) {
            execute_task(task);
            continue;
        }

        idle_wait_count_.fetch_add(1, std::memory_order_relaxed);
        sleeping_workers_.fetch_add(1, std::memory_order_relaxed);
        if (pending_tasks_.load(std::memory_order_acquire) == 0 && !st.stop_requested()) {
            task_signal_.acquire();
        }
        sleeping_workers_.fetch_sub(1, std::memory_order_relaxed);
    }

    tls_current_pool_ = nullptr;
    tls_worker_index_ = static_cast<std::size_t>(-1);
}

} // namespace bee
