/**
 * @File WhenAny.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <stop_token>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Base/Core/MoveOnlyFunction.hpp"
#include "Base/Diagnostics/Check.hpp"
#include "Task/Core/Task.hpp"

namespace bee
{

// =========================================================================
// WhenAnyResult<T> — 同构 when_any 的结果类型
// =========================================================================

/// 同构任务向量 when_any 的结果。
template <typename T>
struct WhenAnyResult
{
    size_t index; ///< 最先完成的任务的索引。
    T value;      ///< 其结果值。
};

// =========================================================================
// when_any() — 竞争组合子
// =========================================================================

namespace detail
{

    /// when_any 的共享记账数据：追踪获胜者和剩余任务计数。
    template <typename ResultType>
    struct WhenAnyControl
    {
        std::shared_ptr<SharedState<ResultType>> combined;
        std::atomic<size_t> remaining;
        std::atomic<bool> first_claimed{false};
        std::optional<ResultType> winner_result;
        std::exception_ptr winner_ex;

        explicit WhenAnyControl(std::shared_ptr<SharedState<ResultType>> c, size_t n)
            : combined(std::move(c))
            , remaining(n)
        {
        }

        auto try_finish() -> void
        {
            if (remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                if (winner_ex) {
                    combined->fail(winner_ex);
                } else if (winner_result.has_value()) {
                    combined->complete(std::move(*winner_result));
                } else {
                    combined->cancel();
                }
            }
        }
    };

} // namespace detail

/// 同构 when_any：等待 vector 中第一个完成或失败的任务。
/// 当第一个结果到达时调用 source.request_stop()，然后等待所有
/// 任务到达终态后再 resolve 合并任务。
/// @note T 不可为 void——void 任务请使用变参版 when_any。
/// @pre !tasks.empty()
template <typename T>
auto when_any(std::stop_source& source, std::vector<Task<T>> tasks) -> Task<WhenAnyResult<T>>
{
    static_assert(!std::is_void_v<T>, "Use variadic when_any() for void tasks.");
    BEE_CHECK(!tasks.empty());

    auto n        = tasks.size();
    auto combined = std::make_shared<detail::SharedState<WhenAnyResult<T>>>();
    auto ctrl     = std::make_shared<detail::WhenAnyControl<WhenAnyResult<T>>>(combined, n);

    auto src_copy = source;

    for (size_t i = 0; i < n; ++i) {
        auto prev_state = tasks[i].state_;

        auto handler = [prev = prev_state, i, src_copy, ctrl]() mutable {
            auto s = prev->state.load(std::memory_order_acquire);

            if (s == TaskState::Completed || s == TaskState::Failed) {
                bool expected = false;
                if (ctrl->first_claimed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                    if (s == TaskState::Completed) {
                        ctrl->winner_result.emplace(WhenAnyResult<T>{i, std::move(*prev->result)});
                    } else {
                        ctrl->winner_ex = prev->exception;
                    }
                    src_copy.request_stop();
                }
            }
            ctrl->try_finish();
        };

        auto handler_fn = MoveOnlyFunction<void()>(std::move(handler));
        if (!detail::attach_when_all_handler(prev_state, handler_fn)) {
            handler_fn();
        }
    }

    return Task<WhenAnyResult<T>>(combined);
}

/// 异构 when_any：等待 N 个不同类型任务中的第一个。
/// 返回 std::variant——使用 .index() 判断哪个任务胜出。
/// 当第一个结果到达时调用 source.request_stop()。
/// @pre sizeof...(Ts) >= 1
template <typename... Ts>
auto when_any(std::stop_source& source, Task<Ts>&&... tasks)
    -> Task<std::variant<typename Task<Ts>::value_type...>>
{
    using ResultType = std::variant<typename Task<Ts>::value_type...>;
    constexpr auto N = sizeof...(Ts);
    static_assert(N >= 1, "when_any requires at least one task.");

    auto combined = std::make_shared<detail::SharedState<ResultType>>();
    auto ctrl     = std::make_shared<detail::WhenAnyControl<ResultType>>(combined, N);
    auto src_copy = source;

    auto task_states = std::make_tuple(std::move(tasks.state_)...);

    [&]<size_t... Is>(std::index_sequence<Is...>) {
        auto attach_one = [&]<size_t I>(std::integral_constant<size_t, I>) {
            using ElemType = std::tuple_element_t<I, std::tuple<Ts...>>;
            auto& prev     = std::get<I>(task_states);

            auto handler = [prev, src_copy, ctrl]() mutable {
                auto s = prev->state.load(std::memory_order_acquire);

                if (s == TaskState::Completed || s == TaskState::Failed) {
                    bool expected = false;
                    if (ctrl->first_claimed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                        if (s == TaskState::Completed) {
                            if constexpr (std::is_void_v<ElemType>) {
                                ctrl->winner_result.emplace(std::in_place_index<I>, std::monostate{});
                            } else {
                                ctrl->winner_result.emplace(std::in_place_index<I>, std::move(*prev->result));
                            }
                        } else {
                            ctrl->winner_ex = prev->exception;
                        }
                        src_copy.request_stop();
                    }
                }
                ctrl->try_finish();
            };

            auto handler_fn = MoveOnlyFunction<void()>(std::move(handler));
            if (!detail::attach_when_all_handler(prev, handler_fn)) {
                handler_fn();
            }
        };

        (attach_one(std::integral_constant<size_t, Is>{}), ...);
    }(std::index_sequence_for<Ts...>{});

    return Task<ResultType>(combined);
}

} // namespace bee
