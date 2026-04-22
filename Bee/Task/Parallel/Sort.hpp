/**
 * @File Sort.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <stop_token>
#include <stdexcept>

#include "Task/Core/Scheduler.hpp"
#include "Task/Parallel/Partitioner.hpp"

namespace bee
{

namespace detail
{

    /// 计算 ceil(log2(n))。当 n <= 1 时返回 0。
    [[nodiscard]] constexpr auto log2_ceil(std::size_t n) noexcept -> std::size_t
    {
        if (n <= 1)
            return 0;
        return static_cast<std::size_t>(std::bit_width(n - 1));
    }

    /// 递归并行归并排序。
    /// 左半区提交到 Scheduler，右半区在当前线程递归处理。
    template <Scheduler S, typename It, typename Comp>
    auto parallel_merge_sort(S& scheduler, It first, It last, Comp comp, std::size_t depth, std::size_t max_depth) -> void
    {
        const auto n = std::distance(first, last);
        if (n < static_cast<std::ptrdiff_t>(kParallelThreshold) || depth >= max_depth) {
            std::sort(first, last, comp);
            return;
        }

        auto mid = first + n / 2;

        // 左半区通过 spawn_task 投递到 Scheduler 线程
        auto left_task = spawn_task(scheduler, [&scheduler, first, mid, comp, depth, max_depth]() {
            parallel_merge_sort(scheduler, first, mid, comp, depth + 1, max_depth);
        });

        // 右半区在当前线程递归处理
        // 确保 left_task 始终被 join 以避免数据竞争
        std::exception_ptr right_ex;
        try {
            parallel_merge_sort(scheduler, mid, last, comp, depth + 1, max_depth);
        } catch (...) {
            right_ex = std::current_exception();
        }

        // 安全 join：捕获左侧异常而非直接抛出，确保双侧异常均被处理
        std::exception_ptr left_ex;
        try {
            left_task.get();
        } catch (...) {
            left_ex = std::current_exception();
        }

        if (left_ex)
            std::rethrow_exception(left_ex);
        if (right_ex)
            std::rethrow_exception(right_ex);

        std::inplace_merge(first, mid, last, comp);
    }

    /// 带取消支持的递归并行归并排序。
    template <Scheduler S, typename It, typename Comp>
    auto parallel_merge_sort(S& scheduler, It first, It last, Comp comp, std::size_t depth, std::size_t max_depth, std::stop_token token) -> void
    {
        if (token.stop_requested())
            return;

        const auto n = std::distance(first, last);
        if (n < static_cast<std::ptrdiff_t>(kParallelThreshold) || depth >= max_depth) {
            std::sort(first, last, comp);
            return;
        }

        auto mid = first + n / 2;

        auto left_task = spawn_task(scheduler, [&scheduler, first, mid, comp, depth, max_depth, token]() {
            parallel_merge_sort(scheduler, first, mid, comp, depth + 1, max_depth, token);
        });

        std::exception_ptr right_ex;
        try {
            parallel_merge_sort(scheduler, mid, last, comp, depth + 1, max_depth, token);
        } catch (...) {
            right_ex = std::current_exception();
        }

        std::exception_ptr left_ex;
        try {
            left_task.get();
        } catch (...) {
            left_ex = std::current_exception();
        }

        if (left_ex)
            std::rethrow_exception(left_ex);
        if (right_ex)
            std::rethrow_exception(right_ex);

        if (!token.stop_requested())
            std::inplace_merge(first, mid, last, comp);
    }

} // namespace detail

// =========================================================================
// parallel_sort — 迭代器接口
// =========================================================================

/// 使用 operator< 并行排序 [first, last)。
/// 要求 RandomAccessIterator。不稳定排序。
template <Scheduler S, std::random_access_iterator It>
auto parallel_sort(S& scheduler, It first, It last) -> void
{
    parallel_sort(scheduler, first, last, std::less<>{});
}

/// 使用自定义比较器排序。
template <Scheduler S, std::random_access_iterator It, typename Comp>
auto parallel_sort(S& scheduler, It first, It last, Comp comp) -> void
{
    const auto n = std::distance(first, last);
    if (n < static_cast<std::ptrdiff_t>(detail::kParallelThreshold)) {
        std::sort(first, last, comp);
        return;
    }

    const std::size_t max_depth = detail::log2_ceil(scheduler.thread_count()) + 1;
    detail::parallel_merge_sort(scheduler, first, last, comp, 0, max_depth);
}

/// 带取消支持的并行排序。
template <Scheduler S, std::random_access_iterator It, typename Comp>
auto parallel_sort(S& scheduler, It first, It last, Comp comp, std::stop_token token) -> void
{
    if (token.stop_requested())
        throw std::runtime_error("Operation cancelled");

    const auto n = std::distance(first, last);
    if (n < static_cast<std::ptrdiff_t>(detail::kParallelThreshold)) {
        std::sort(first, last, comp);
        return;
    }

    const std::size_t max_depth = detail::log2_ceil(scheduler.thread_count()) + 1;
    detail::parallel_merge_sort(scheduler, first, last, comp, 0, max_depth, token);

    if (token.stop_requested())
        throw std::runtime_error("Operation cancelled");
}

// =========================================================================
// parallel_sort — Ranges 接口
// =========================================================================

/// Ranges 版本：使用 operator< 排序。
template <Scheduler S, std::ranges::random_access_range R>
auto parallel_sort(S& scheduler, R&& range) -> void
{
    parallel_sort(scheduler, std::ranges::begin(range), std::ranges::end(range));
}

/// Ranges 版本：使用自定义比较器排序。
template <Scheduler S, std::ranges::random_access_range R, typename Comp>
auto parallel_sort(S& scheduler, R&& range, Comp comp) -> void
{
    parallel_sort(scheduler, std::ranges::begin(range), std::ranges::end(range), comp);
}

/// Ranges 版本 + 取消支持。
template <Scheduler S, std::ranges::random_access_range R, typename Comp>
auto parallel_sort(S& scheduler, R&& range, Comp comp, std::stop_token token) -> void
{
    parallel_sort(scheduler, std::ranges::begin(range), std::ranges::end(range), comp, token);
}

} // namespace bee
