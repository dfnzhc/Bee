/**
 * @File Submit.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#pragma once

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

} // namespace bee
