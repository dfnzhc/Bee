/**
 * @File AsyncTask.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <coroutine>
#include <exception>
#include <optional>
#include <semaphore>
#include <type_traits>
#include <utility>
#include <variant>

#include "Base/Core/Defines.hpp"
#include "Base/Diagnostics/Check.hpp"

namespace bee
{

// =========================================================================
// AsyncTask<T> — 惰性协程返回类型
// =========================================================================

/// 惰性协程：创建时挂起，仅在通过 .get()、.wait() 或 co_await 消费时才启动。
///
/// @note 仅可移动，一次性使用。必须通过 .get() 或 co_await 恰好消费一次。
///       销毁未消费的 AsyncTask 会销毁协程帧。
template <typename T>
class [[nodiscard]] AsyncTask
{
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    AsyncTask() = default;

    AsyncTask(AsyncTask&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr))
    {
    }

    auto operator=(AsyncTask&& other) noexcept -> AsyncTask&
    {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    ~AsyncTask()
    {
        if (handle_) {
            handle_.destroy();
        }
    }

    AsyncTask(const AsyncTask&)                    = delete;
    auto operator=(const AsyncTask&) -> AsyncTask& = delete;

    // -----------------------------------------------------------------
    // 阻塞访问（供非协程调用者使用）
    // -----------------------------------------------------------------

    /// 启动协程（若尚未启动）并阻塞直到 co_return。
    /// 重新抛出协程体中的任何异常。
    auto get() -> T
    {
        BEE_CHECK(handle_ != nullptr);
        start_if_needed();
        wait_until_done();

        if (handle_.promise().exception) {
            std::rethrow_exception(handle_.promise().exception);
        }
        if constexpr (!std::is_void_v<T>) {
            return std::move(*handle_.promise().result);
        }
    }

    /// 启动协程（若尚未启动）并阻塞直到 co_return。
    /// 不返回结果——用于副作用协程。
    /// @note 与 Task<T>::wait() 不同，此方法会重新抛出协程体中抛出的异常。
    auto wait() -> void
    {
        BEE_CHECK(handle_ != nullptr);
        start_if_needed();
        wait_until_done();

        if (handle_.promise().exception) {
            std::rethrow_exception(handle_.promise().exception);
        }
    }

    /// 协程是否已完成（已到达 co_return 或抛出异常）。
    [[nodiscard]] auto is_ready() const noexcept -> bool
    {
        return handle_ && handle_.promise().done.load(std::memory_order_acquire);
    }

    /// AsyncTask 是否持有有效的协程句柄。
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    // -----------------------------------------------------------------
    // 协程访问（供协程调用者使用）
    // -----------------------------------------------------------------

    /// co_await AsyncTask：通过对称转移启动它（若为惰性状态），
    /// 挂起调用者，完成后返回结果。
    auto operator co_await()
    {
        BEE_CHECK(handle_ != nullptr);

        struct AsyncTaskAwaiter
        {
            handle_type handle_;

            [[nodiscard]] auto await_ready() const noexcept -> bool
            {
                return handle_.done();
            }

            auto await_suspend(std::coroutine_handle<> caller) -> std::coroutine_handle<>
            {
                [[maybe_unused]] bool was_started = handle_.promise().started.exchange(true, std::memory_order_acq_rel);
                BEE_ASSERT(!was_started);
                handle_.promise().continuation = caller;
                return handle_; // 对称转移——恢复 AsyncTask 协程
            }

            auto await_resume() -> T
            {
                if (handle_.promise().exception) {
                    std::rethrow_exception(handle_.promise().exception);
                }
                if constexpr (!std::is_void_v<T>) {
                    return std::move(*handle_.promise().result);
                }
            }
        };

        return AsyncTaskAwaiter{handle_};
    }

private:
    explicit AsyncTask(handle_type h)
        : handle_(h)
    {
    }

    auto start_if_needed() -> void
    {
        if (!handle_.promise().started.exchange(true, std::memory_order_acq_rel)) {
            handle_.resume();
        }
    }

    auto wait_until_done() -> void
    {
        /// 安全保证：若 FinalAwaiter 在检查与获取操作之间释放信号量，获取操作会立即返回。
        if (!handle_.promise().done.load(std::memory_order_acquire)) {
            handle_.promise().ready.acquire();
        }
    }

    handle_type handle_{nullptr};
};

// =========================================================================
// promise_type
// =========================================================================

namespace detail
{

    /// 自定义 final awaiter：通知 .get() 等待者，并通过对称转移恢复
    /// co_await 调用者。
    struct AsyncTaskFinalAwaiter
    {
        [[nodiscard]] auto await_ready() const noexcept -> bool
        {
            return false;
        }

        template <typename Promise>
        auto await_suspend(std::coroutine_handle<Promise> h) noexcept -> std::coroutine_handle<>
        {
            auto& promise = h.promise();

            promise.done.store(true, std::memory_order_release);

            // 通过对称转移恢复 co_await 调用者。
            if (promise.continuation) {
                return promise.continuation;
            }
            
            // 无协程等待调用者 —— 为调用 .get ()/.wait () 的等待者释放信号量。
            promise.ready.release();
            return std::noop_coroutine();
        }

        auto await_resume() noexcept -> void
        {
        }
    };

    // MSVC 要求 return_value 和 return_void 位于不同类型中。
    // 将返回方法拆分到基类中以避免冲突。

    template <typename T>
    struct AsyncTaskPromiseReturn
    {
        std::optional<T> result{};

        auto return_value(T value) -> void
        {
            result.emplace(std::move(value));
        }
    };

    template <>
    struct AsyncTaskPromiseReturn<void>
    {
        auto return_void() -> void
        {
        }
    };

} // namespace detail

template <typename T>
struct AsyncTask<T>::promise_type : detail::AsyncTaskPromiseReturn<T>
{
    std::exception_ptr exception;

    std::coroutine_handle<> continuation{nullptr};
    std::binary_semaphore ready{0};
    std::atomic<bool> done{false};
    std::atomic<bool> started{false};

    auto get_return_object() -> AsyncTask
    {
        return AsyncTask{handle_type::from_promise(*this)};
    }

    auto initial_suspend() noexcept -> std::suspend_always
    {
        return {}; // 惰性——立即挂起
    }

    auto final_suspend() noexcept -> detail::AsyncTaskFinalAwaiter
    {
        return {};
    }

    auto unhandled_exception() -> void
    {
        exception = std::current_exception();
    }
};

} // namespace bee
