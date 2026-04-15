/**
 * @File Submit.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include <mutex>
#include <optional>
#include <stop_token>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

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
auto submit_cancellable(ThreadPool& pool, Fn&& fn, std::stop_source& source) -> Task<std::invoke_result_t<Fn, std::stop_token>>
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

// =========================================================================
// when_all() — fork-join 组合子
// =========================================================================

namespace detail
{

    /// 将完成处理器挂载到 SharedState。
    /// 处理器在任意终态触发。
    /// 若已处于终态则返回 false（调用者应手动执行 handler）。
    template <typename T>
    auto attach_when_all_handler(std::shared_ptr<SharedState<T>>& prev_state, MoveOnlyFunction<void()>& handler) -> bool
    {
        std::lock_guard lock(prev_state->mutex);
        BEE_CHECK(!prev_state->has_continuation);
        if (prev_state->is_terminal()) {
            return false; // handler 未被消费——调用者仍可执行
        }
        prev_state->continuation     = std::move(handler);
        prev_state->has_continuation = true;
        return true;
    }

} // namespace detail

/// 异构 when_all：等待所有任务完成，返回结果 tuple。
/// 使用 value_type（void 任务为 std::monostate），保证 tuple 始终合法。
/// 若任一任务失败或取消，合并 Task 以第一个捕获的异常失败。
template <typename... Ts>
auto when_all(Task<Ts>&&... tasks) -> Task<std::tuple<typename Task<Ts>::value_type...>>
{
    using ResultType = std::tuple<typename Task<Ts>::value_type...>;
    constexpr auto N = sizeof...(Ts);

    auto combined = std::make_shared<detail::SharedState<ResultType>>();

    if constexpr (N == 0) {
        combined->complete(ResultType{});
        return Task<ResultType>(combined);
    } else {
        auto remaining = std::make_shared<std::atomic<size_t>>(N);
        auto results   = std::make_shared<ResultType>();
        auto first_ex  = std::make_shared<std::exception_ptr>();
        auto ex_mutex  = std::make_shared<std::mutex>();

        auto try_finish = [combined, remaining, results, first_ex]() {
            if (remaining->fetch_sub(1, std::memory_order_acq_rel) == 1) {
                if (*first_ex) {
                    combined->fail(*first_ex);
                } else {
                    combined->complete(std::move(*results));
                }
            }
        };

        // 在 fold 之前从 tasks 中提取 shared_ptr（tasks 为右值引用）。
        auto task_states = std::make_tuple(std::move(tasks.state_)...);

        [&]<size_t... Is>(std::index_sequence<Is...>) {
            auto attach_one = [&]<size_t I>(std::integral_constant<size_t, I>) {
                using ElemType = std::tuple_element_t<I, std::tuple<Ts...>>;
                auto& prev     = std::get<I>(task_states);

                auto handler = [prev, results, first_ex, ex_mutex, try_finish]() mutable {
                    auto s = prev->state.load(std::memory_order_acquire);
                    if (s == TaskState::Completed) {
                        if constexpr (!std::is_void_v<ElemType>) {
                            std::get<I>(*results) = std::move(*prev->result);
                        }
                    } else {
                        std::lock_guard lock(*ex_mutex);
                        if (!*first_ex) {
                            if (s == TaskState::Failed) {
                                *first_ex = prev->exception;
                            } else {
                                *first_ex = std::make_exception_ptr(std::runtime_error("Task was cancelled"));
                            }
                        }
                    }
                    try_finish();
                };

                auto handler_fn = MoveOnlyFunction<void()>(std::move(handler));
                if (!detail::attach_when_all_handler(prev, handler_fn)) {
                    // 已处于终态——handler_fn 未被消费，立即执行。
                    handler_fn();
                }
            };

            (attach_one(std::integral_constant<size_t, Is>{}), ...);
        }(std::index_sequence_for<Ts...>{});

        return Task<ResultType>(combined);
    }
}

/// 同构 when_all：等待 vector 中所有任务完成，返回结果 vector。
/// T 不可为 void（void 任务请使用变参版 when_all）。
template <typename T>
auto when_all(std::vector<Task<T>> tasks) -> Task<std::vector<T>>
{
    static_assert(!std::is_void_v<T>, "Use variadic when_all() for void tasks.");

    auto n        = tasks.size();
    auto combined = std::make_shared<detail::SharedState<std::vector<T>>>();

    if (n == 0) {
        combined->complete(std::vector<T>{});
        return Task<std::vector<T>>(combined);
    }

    auto remaining = std::make_shared<std::atomic<size_t>>(n);
    auto results   = std::make_shared<std::vector<std::optional<T>>>(n);
    auto first_ex  = std::make_shared<std::exception_ptr>();
    auto ex_mutex  = std::make_shared<std::mutex>();

    auto try_finish = [combined, remaining, results, first_ex, n]() {
        if (remaining->fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (*first_ex) {
                combined->fail(*first_ex);
            } else {
                std::vector<T> final_results;
                final_results.reserve(n);
                for (auto& opt : *results) {
                    final_results.push_back(std::move(*opt));
                }
                combined->complete(std::move(final_results));
            }
        }
    };

    for (size_t i = 0; i < n; ++i) {
        auto prev_state = tasks[i].state_;

        auto handler = [prev = prev_state, results, first_ex, ex_mutex, try_finish, i]() mutable {
            auto s = prev->state.load(std::memory_order_acquire);
            if (s == TaskState::Completed) {
                (*results)[i].emplace(std::move(*prev->result));
            } else {
                std::lock_guard lock(*ex_mutex);
                if (!*first_ex) {
                    if (s == TaskState::Failed) {
                        *first_ex = prev->exception;
                    } else {
                        *first_ex = std::make_exception_ptr(std::runtime_error("Task was cancelled"));
                    }
                }
            }
            try_finish();
        };

        auto handler_fn = MoveOnlyFunction<void()>(std::move(handler));
        if (!detail::attach_when_all_handler(prev_state, handler_fn)) {
            // 已处于终态——handler_fn 未被消费，立即执行。
            handler_fn();
        }
    }

    return Task<std::vector<T>>(combined);
}

} // namespace bee
