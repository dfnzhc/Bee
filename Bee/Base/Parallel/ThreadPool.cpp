/**
 * @File ThreadPool.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 全局线程池实现。
 */

#include "Base/Parallel/ThreadPool.hpp"

#include <algorithm>
#include <utility>

namespace bee::parallel {

ThreadPool::ThreadPool(std::size_t thread_count)
{
    workers_.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back([this]() noexcept { worker_loop(); });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard lk(mu_);
        stop_.store(true, std::memory_order_release);
    }
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable())
            t.join();
    }
}

void ThreadPool::submit(std::function<void()> task)
{
    {
        std::lock_guard lk(mu_);
        queue_.emplace(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::worker_loop() noexcept
{
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock lk(mu_);
            cv_.wait(lk, [this] {
                return stop_.load(std::memory_order_acquire) || !queue_.empty();
            });
            if (queue_.empty() && stop_.load(std::memory_order_acquire))
                return;
            task = std::move(queue_.front());
            queue_.pop();
        }
        // 异常由调用者约定在 task 内自行捕获，避免越过 worker 边界终止进程。
        task();
    }
}

ThreadPool& ThreadPool::instance() noexcept
{
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    // 主线程参与最后一块，减一以避免超订阅。
    const std::size_t n = hw <= 1 ? 0 : static_cast<std::size_t>(hw - 1);
    static ThreadPool pool(n);
    return pool;
}

} // namespace bee::parallel
