/**
 * @File Reduce.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <iterator>
#include <latch>
#include <numeric>
#include <ranges>
#include <stop_token>
#include <stdexcept>
#include <vector>

#include "Task/Parallel/Partitioner.hpp"

namespace bee
{

// =========================================================================
// parallel_reduce — 迭代器接口
// =========================================================================

/// 以 init 为初始值，用二元操作 op 归约 [first, last)。
/// op 必须满足结合律和交换律。
template <Scheduler S, std::random_access_iterator It, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_reduce(S& scheduler, It first, It last, T init, BinaryOp op) -> T
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0)
        return init;

    if (n < detail::kParallelThreshold)
        return std::reduce(first, last, init, op);

    auto                            chunks = detail::partition(n, scheduler.thread_count());
    const auto                      count  = chunks.size();
    std::vector<T>                  partial_results(count);
    std::vector<std::exception_ptr> exceptions(count);
    std::latch                      done(static_cast<std::ptrdiff_t>(count));

    detail::safe_post_loop(scheduler, count, done, [&](std::size_t i) {
        try {
            auto it  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
            auto end = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
            T    acc = *it;
            ++it;
            for (; it != end; ++it) {
                acc = op(acc, *it);
            }
            partial_results[i] = std::move(acc);
        }
        catch (...) {
            exceptions[i] = std::current_exception();
        }
        done.count_down();
    });

    detail::rethrow_first(exceptions);

    T result = init;
    for (auto& partial : partial_results) {
        result = op(result, std::move(partial));
    }
    return result;
}

/// 带取消支持的并行归约。
template <Scheduler S, std::random_access_iterator It, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_reduce(S& scheduler, It first, It last, T init, BinaryOp op, std::stop_token token) -> T
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0)
        return init;

    if (n < detail::kParallelThreshold) {
        T acc = init;
        for (auto it = first; it != last; ++it) {
            if (token.stop_requested())
                throw std::runtime_error("Operation cancelled");
            acc = op(acc, *it);
        }
        return acc;
    }

    auto                            chunks = detail::partition(n, scheduler.thread_count());
    const auto                      count  = chunks.size();
    std::vector<T>                  partial_results(count);
    std::vector<std::exception_ptr> exceptions(count);
    std::atomic<bool>               cancelled{false};
    std::latch                      done(static_cast<std::ptrdiff_t>(count));

    detail::safe_post_loop(scheduler, count, done, [&](std::size_t i) {
        if (token.stop_requested()) {
            cancelled.store(true, std::memory_order_relaxed);
            done.count_down();
            return;
        }
        try {
            auto it  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
            auto end = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
            T    acc = *it;
            ++it;
            for (; it != end; ++it) {
                if (token.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    break;
                }
                acc = op(acc, *it);
            }
            partial_results[i] = std::move(acc);
        }
        catch (...) {
            exceptions[i] = std::current_exception();
        }
        done.count_down();
    });

    detail::rethrow_first(exceptions);

    if (cancelled.load(std::memory_order_acquire))
        throw std::runtime_error("Operation cancelled");

    T result = init;
    for (auto& partial : partial_results) {
        result = op(result, std::move(partial));
    }
    return result;
}

// =========================================================================
// parallel_reduce — Ranges 接口
// =========================================================================

template <Scheduler S, std::ranges::random_access_range R, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_reduce(S& scheduler, R&& range, T init, BinaryOp op) -> T
{
    return parallel_reduce(scheduler, std::ranges::begin(range), std::ranges::end(range), init, op);
}

template <Scheduler S, std::ranges::random_access_range R, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_reduce(S& scheduler, R&& range, T init, BinaryOp op, std::stop_token token) -> T
{
    return parallel_reduce(scheduler, std::ranges::begin(range), std::ranges::end(range), init, op, token);
}

// =========================================================================
// parallel_transform_reduce — 迭代器接口
// =========================================================================

/// 先变换每个元素再归约。融合执行——无中间分配。
/// reduce_op 必须满足结合律和交换律。
template <Scheduler S, std::random_access_iterator It, typename T, typename ReduceOp, typename TransformOp>
[[nodiscard]] auto parallel_transform_reduce(
    S& scheduler, It first, It last, T init, ReduceOp reduce_op, TransformOp transform_op) -> T
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0)
        return init;

    if (n < detail::kParallelThreshold)
        return std::transform_reduce(first, last, init, reduce_op, transform_op);

    auto                            chunks = detail::partition(n, scheduler.thread_count());
    const auto                      count  = chunks.size();
    std::vector<T>                  partial_results(count);
    std::vector<std::exception_ptr> exceptions(count);
    std::latch                      done(static_cast<std::ptrdiff_t>(count));

    detail::safe_post_loop(scheduler, count, done, [&](std::size_t i) {
        try {
            auto it  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
            auto end = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
            T    acc = transform_op(*it);
            ++it;
            for (; it != end; ++it) {
                acc = reduce_op(acc, transform_op(*it));
            }
            partial_results[i] = std::move(acc);
        }
        catch (...) {
            exceptions[i] = std::current_exception();
        }
        done.count_down();
    });

    detail::rethrow_first(exceptions);

    T result = init;
    for (auto& partial : partial_results) {
        result = reduce_op(result, std::move(partial));
    }
    return result;
}

/// 带取消支持的并行变换归约。
template <Scheduler S, std::random_access_iterator It, typename T, typename ReduceOp, typename TransformOp>
[[nodiscard]] auto parallel_transform_reduce(
    S& scheduler, It first, It last, T init, ReduceOp reduce_op, TransformOp transform_op, std::stop_token token) -> T
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0)
        return init;

    if (n < detail::kParallelThreshold) {
        T acc = init;
        for (auto it = first; it != last; ++it) {
            if (token.stop_requested())
                throw std::runtime_error("Operation cancelled");
            acc = reduce_op(acc, transform_op(*it));
        }
        return acc;
    }

    auto                            chunks = detail::partition(n, scheduler.thread_count());
    const auto                      count  = chunks.size();
    std::vector<T>                  partial_results(count);
    std::vector<std::exception_ptr> exceptions(count);
    std::atomic<bool>               cancelled{false};
    std::latch                      done(static_cast<std::ptrdiff_t>(count));

    detail::safe_post_loop(scheduler, count, done, [&](std::size_t i) {
        if (token.stop_requested()) {
            cancelled.store(true, std::memory_order_relaxed);
            done.count_down();
            return;
        }
        try {
            auto it  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
            auto end = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
            T    acc = transform_op(*it);
            ++it;
            for (; it != end; ++it) {
                if (token.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    break;
                }
                acc = reduce_op(acc, transform_op(*it));
            }
            partial_results[i] = std::move(acc);
        }
        catch (...) {
            exceptions[i] = std::current_exception();
        }
        done.count_down();
    });

    detail::rethrow_first(exceptions);

    if (cancelled.load(std::memory_order_acquire))
        throw std::runtime_error("Operation cancelled");

    T result = init;
    for (auto& partial : partial_results) {
        result = reduce_op(result, std::move(partial));
    }
    return result;
}

// =========================================================================
// parallel_transform_reduce — Ranges 接口
// =========================================================================

template <Scheduler S, std::ranges::random_access_range R, typename T, typename ReduceOp, typename TransformOp>
[[nodiscard]] auto parallel_transform_reduce(S& scheduler, R&& range, T init, ReduceOp reduce_op, TransformOp transform_op) -> T
{
    return parallel_transform_reduce(scheduler, std::ranges::begin(range), std::ranges::end(range), init, reduce_op, transform_op);
}

template <Scheduler S, std::ranges::random_access_range R, typename T, typename ReduceOp, typename TransformOp>
[[nodiscard]] auto parallel_transform_reduce(S& scheduler, R&& range, T init, ReduceOp reduce_op, TransformOp transform_op, std::stop_token token) -> T
{
    return parallel_transform_reduce(scheduler, std::ranges::begin(range), std::ranges::end(range), init, reduce_op, transform_op, token);
}

} // namespace bee
