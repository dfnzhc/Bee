/**
 * @File SharedState.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <stop_token>
#include <type_traits>
#include <utility>

#include "Base/Core/Defines.hpp"
#include "Base/Core/MoveOnlyFunction.hpp"
#include "Task/Core/TaskState.hpp"

namespace bee::detail
{

// =========================================================================
// SharedState<T> — Task<T> 的内部结果/continuation存储
// =========================================================================

template <typename T>
struct SharedState
{
    // 结果存储：非 void 类型使用 std::optional<T>，void 类型使用空标签。
    struct Empty
    {
    };

    using ResultStorage = std::conditional_t<std::is_void_v<T>, Empty, std::optional<T>>;

    mutable std::mutex              mutex;
    mutable std::condition_variable cv;
    std::atomic<TaskState>          state{TaskState::Pending};

    BEE_NO_UNIQUE_ADDRESS ResultStorage result{};
    std::exception_ptr                  exception;
    std::stop_token                     stop_token;

    // continuation：至多一个，由 then() 或 when_all() 设置。
    // 在任何终态（Completed、Failed、Cancelled）触发。
    MoveOnlyFunction<void()> continuation;
    bool                     has_continuation{false};

    // -----------------------------------------------------------------
    // 状态转换
    // -----------------------------------------------------------------

    auto set_running() -> void
    {
        state.store(TaskState::Running, std::memory_order_release);
    }

    template <typename U = T>
        requires(!std::is_void_v<U>)
    auto complete(U&& value) -> void
    {
        terminate([&] {
            result.emplace(std::forward<U>(value));
            state.store(TaskState::Completed, std::memory_order_release);
        });
    }

    auto complete() -> void
        requires std::is_void_v<T>
    {
        terminate([&] { state.store(TaskState::Completed, std::memory_order_release); });
    }

    auto fail(std::exception_ptr ep) -> void
    {
        terminate([&] {
            exception = std::move(ep);
            state.store(TaskState::Failed, std::memory_order_release);
        });
    }

    auto cancel() -> void
    {
        terminate([&] { state.store(TaskState::Cancelled, std::memory_order_release); });
    }

    // -----------------------------------------------------------------
    // 查询
    // -----------------------------------------------------------------

    [[nodiscard]] auto is_terminal() const noexcept -> bool
    {
        auto s = state.load(std::memory_order_acquire);
        return s == TaskState::Completed || s == TaskState::Failed || s == TaskState::Cancelled;
    }

    // -----------------------------------------------------------------
    // 阻塞等待
    // -----------------------------------------------------------------

    auto wait() const -> void
    {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this] { return is_terminal(); });
    }

    template <typename Rep, typename Period>
    auto wait_for(std::chrono::duration<Rep, Period> timeout) const -> TaskState
    {
        std::unique_lock lock(mutex);
        cv.wait_for(lock, timeout, [this] { return is_terminal(); });
        return state.load(std::memory_order_acquire);
    }

private:
    // 终态转换公共辅助：加锁设置状态、提取 continuation、通知 CV、在锁外执行 continuation。
    template <typename SetupFn>
    auto terminate(SetupFn&& setup) -> void
    {
        MoveOnlyFunction<void()> cont;
        {
            std::lock_guard lock(mutex);
            setup();
            if (has_continuation) {
                cont             = std::move(continuation);
                has_continuation = false;
            }
        }
        cv.notify_all();
        if (cont) {
            cont();
        }
    }
};

// =========================================================================
// Continuation 类型辅助工具
// =========================================================================

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

/// 调用 Continuation 函数并完成后继共享状态。
/// 处理 T 和 R 的 void/non-void 四种组合。
template <typename T, typename R, typename Fn>
auto invoke_continuation(Fn& fn, SharedState<T>* prev, SharedState<R>* next) -> void
{
    if constexpr (std::is_void_v<T> && std::is_void_v<R>) {
        fn();
        next->complete();
    } else if constexpr (std::is_void_v<T>) {
        next->complete(fn());
    } else if constexpr (std::is_void_v<R>) {
        fn(std::move(*prev->result));
        next->complete();
    } else {
        next->complete(fn(std::move(*prev->result)));
    }
}

} // namespace bee::detail
