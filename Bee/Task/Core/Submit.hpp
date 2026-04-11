/**
 * @File Submit.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include <stop_token>
#include <type_traits>
#include <utility>

#include "Task/Core/Task.hpp"
#include "Concurrency/Thread/ThreadPool.hpp"

namespace bee
{

// =========================================================================
// submit() — 将工作投递到 ThreadPool，返回 Task<T>
// =========================================================================

/// 将可调用对象提交到线程池，返回持有其结果的 Task。
template <typename Fn>
auto submit(ThreadPool& pool, Fn&& fn) -> Task<std::invoke_result_t<Fn>>
{
    using R = std::invoke_result_t<Fn>;

    auto state = std::make_shared<detail::SharedState<R>>();

    pool.post([state, fn = std::forward<Fn>(fn)]() mutable {
        state->set_running();
        try {
            if constexpr (std::is_void_v<R>) {
                fn();
                state->complete();
            } else {
                state->complete(fn());
            }
        } catch (...) {
            state->fail(std::current_exception());
        }
    });

    return Task<R>(state);
}

/// 带执行前取消检查的提交。
/// 若可调用对象开始执行前 stop_requested() 为 true，则任务直接取消。
template <typename Fn>
auto submit(ThreadPool& pool, Fn&& fn, std::stop_token token) -> Task<std::invoke_result_t<Fn>>
{
    using R = std::invoke_result_t<Fn>;

    auto state        = std::make_shared<detail::SharedState<R>>();
    state->stop_token = token;

    pool.post([state, fn = std::forward<Fn>(fn)]() mutable {
        if (state->stop_token.stop_requested()) {
            state->cancel();
            return;
        }
        state->set_running();
        try {
            if constexpr (std::is_void_v<R>) {
                fn();
                state->complete();
            } else {
                state->complete(fn());
            }
        } catch (...) {
            state->fail(std::current_exception());
        }
    });

    return Task<R>(state);
}

/// 提交接收 stop_token 参数的可调用对象，用于执行中检查取消。
template <typename Fn>
    requires std::is_invocable_v<Fn, std::stop_token>
auto submit_cancellable(ThreadPool& pool, Fn&& fn, std::stop_source& source)
    -> Task<std::invoke_result_t<Fn, std::stop_token>>
{
    using R = std::invoke_result_t<Fn, std::stop_token>;

    auto token        = source.get_token();
    auto state        = std::make_shared<detail::SharedState<R>>();
    state->stop_token = token;

    pool.post([state, fn = std::forward<Fn>(fn), token]() mutable {
        if (token.stop_requested()) {
            state->cancel();
            return;
        }
        state->set_running();
        try {
            if constexpr (std::is_void_v<R>) {
                fn(token);
                state->complete();
            } else {
                state->complete(fn(token));
            }
        } catch (...) {
            state->fail(std::current_exception());
        }
    });

    return Task<R>(state);
}

} // namespace bee
