/**
 * @File ThreadPool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 轻量级全局线程池，为 Base::Parallel 的 fork-join 原语提供工作者线程。
 *        与 Task/AsyncTask/Scheduler 体系解耦，避免高性能数值路径承载调度层开销。
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace bee::parallel {

/// 轻量级固定大小线程池：持久工作线程 + 共享 FIFO 队列。
/// 仅用于 parallel_for 等 fork-join 原语，不承担任务依赖/续延等复杂调度。
class ThreadPool
{
public:
    /// 全局共享实例：size = max(1, hardware_concurrency - 1)。
    /// 主调用线程视为额外的一个执行槽，故此处少开一个线程避免超订阅。
    static ThreadPool& instance() noexcept;

    explicit ThreadPool(std::size_t thread_count);
    ~ThreadPool();

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    [[nodiscard]] std::size_t size() const noexcept { return workers_.size(); }

    /// 提交一个可执行任务。线程安全。任务内不应再次阻塞等待同池任务，
    /// 否则在 worker 数耗尽时可能死锁。
    void submit(std::function<void()> task);

private:
    void worker_loop() noexcept;

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> queue_;
    std::mutex                        mu_;
    std::condition_variable           cv_;
    std::atomic<bool>                 stop_{false};
};

/// 可执行并行度：主线程 + 工作线程数。
[[nodiscard]] inline std::size_t available_parallelism() noexcept
{
    return ThreadPool::instance().size() + 1;
}

} // namespace bee::parallel
