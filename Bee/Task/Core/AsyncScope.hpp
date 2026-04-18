/**
 * @File AsyncScope.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <mutex>
#include <stop_token>

#include "Base/Diagnostics/Check.hpp"
#include "Task/Core/Scheduler.hpp"
#include "Task/Core/Task.hpp"

namespace bee
{

/// AsyncScope 管理动态 spawn 的 Task 生命周期。
///
/// 核心保证：子任务不超过 scope 存活。
///
/// 用法：
///   AsyncScope scope;
///   scope.spawn(pool, some_task(pool));
///   scope.spawn(pool, another_task(pool));
///   scope.join();   // 等待所有 spawn 的任务完成
///   // 或让 ~AsyncScope 自动 join
class AsyncScope
{
public:
    AsyncScope() = default;

    ~AsyncScope()
    {
        // 等待所有任务完成（不重抛异常，避免 std::terminate）
        std::unique_lock lock(cv_mutex_);
        cv_.wait(lock, [this] { return pending_.load(std::memory_order_acquire) == 0; });
    }

    AsyncScope(const AsyncScope&)                    = delete;
    AsyncScope(AsyncScope&&)                         = delete;
    auto operator=(const AsyncScope&) -> AsyncScope& = delete;
    auto operator=(AsyncScope&&) -> AsyncScope&      = delete;

    /// 在 scheduler 上 spawn 一个 Task。Task 的生命周期由 scope 管理。
    template <Scheduler S, typename T>
    auto spawn(S& scheduler, Task<T> task) -> void
    {
        pending_.fetch_add(1, std::memory_order_acq_rel);
        run_detached(scheduler, std::move(task));
    }

    /// 阻塞等待所有已 spawn 的任务完成。
    /// 如果有任务抛出异常，重抛第一个异常。
    auto join() -> void
    {
        {
            std::unique_lock lock(cv_mutex_);
            cv_.wait(lock, [this] { return pending_.load(std::memory_order_acquire) == 0; });
        }
        check_exception();
    }

    /// 请求取消所有已 spawn 的任务。
    auto request_stop() -> void
    {
        stop_source_.request_stop();
    }

    /// 获取 stop_token（子任务可用于检查取消请求）。
    [[nodiscard]] auto get_stop_token() const noexcept -> std::stop_token
    {
        return stop_source_.get_token();
    }

    /// 已 spawn 但未完成的任务数量。
    [[nodiscard]] auto pending_count() const noexcept -> std::size_t
    {
        return pending_.load(std::memory_order_acquire);
    }

    /// 所有已 spawn 的任务是否都已完成。
    [[nodiscard]] auto is_idle() const noexcept -> bool
    {
        return pending_.load(std::memory_order_acquire) == 0;
    }

private:
    std::atomic<std::size_t> pending_{0};
    std::stop_source         stop_source_;
    std::exception_ptr       first_ex_;
    std::mutex               ex_mutex_;
    std::mutex               cv_mutex_;
    std::condition_variable  cv_;

    /// 包装 Task 为 DetachedTask，追踪完成计数
    template <Scheduler S, typename T>
    auto run_detached(S& scheduler, Task<T> task) -> detail::DetachedTask
    {
        try {
            co_await scheduler.schedule();
            co_await std::move(task);
        }
        catch (...) {
            std::lock_guard lock(ex_mutex_);
            if (!first_ex_) {
                first_ex_ = std::current_exception();
            }
        }

        if (pending_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // 最后一个任务完成，通知 join/析构
            cv_.notify_all();
        }
    }

    auto check_exception() -> void
    {
        if (first_ex_) {
            auto ex  = first_ex_;
            first_ex_ = nullptr;
            std::rethrow_exception(ex);
        }
    }
};

} // namespace bee
