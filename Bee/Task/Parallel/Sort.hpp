/**
 * @File Sort.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/12
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <functional>
#include <iterator>
#include <stop_token>
#include <stdexcept>

#include "Task/Parallel/Partitioner.hpp"
#include "Task/Core/Submit.hpp"

namespace bee
{

namespace detail
{

    /// 计算 ceil(log2(n))。当 n <= 1 时返回 0。
    [[nodiscard]] constexpr auto log2_ceil(size_t n) noexcept -> size_t
    {
        if (n <= 1)
            return 0;
        return static_cast<size_t>(std::bit_width(n - 1));
    }

    /// 递归并行归并排序。
    template <typename It, typename Comp>
    auto parallel_merge_sort(ThreadPool& pool, It first, It last, Comp comp, size_t depth, size_t max_depth) -> void
    {
        const auto n = std::distance(first, last);
        if (n < static_cast<std::ptrdiff_t>(kParallelThreshold) || depth >= max_depth) {
            std::sort(first, last, comp);
            return;
        }

        auto mid = first + n / 2;

        // 左半区提交到线程池任务。
        auto left_task =
            bee::submit(pool, [&pool, first, mid, comp, depth, max_depth]() { parallel_merge_sort(pool, first, mid, comp, depth + 1, max_depth); });

        // 右半区在当前线程递归处理；确保 left_task 总是被 join 以避免数据竞争。
        std::exception_ptr right_ex;
        try {
            parallel_merge_sort(pool, mid, last, comp, depth + 1, max_depth);
        } catch (...) {
            right_ex = std::current_exception();
        }

        left_task.get();

        if (right_ex)
            std::rethrow_exception(right_ex);

        std::inplace_merge(first, mid, last, comp);
    }

    /// 带取消支持的递归并行归并排序。
    template <typename It, typename Comp>
    auto parallel_merge_sort(ThreadPool& pool, It first, It last, Comp comp, size_t depth, size_t max_depth, std::stop_token token) -> void
    {
        if (token.stop_requested())
            return;

        const auto n = std::distance(first, last);
        if (n < static_cast<std::ptrdiff_t>(kParallelThreshold) || depth >= max_depth) {
            std::sort(first, last, comp);
            return;
        }

        auto mid = first + n / 2;

        auto left_task = bee::submit(pool, [&pool, first, mid, comp, depth, max_depth, token]() {
            parallel_merge_sort(pool, first, mid, comp, depth + 1, max_depth, token);
        });

        // 确保 left_task 总是被 join 以避免数据竞争。
        std::exception_ptr right_ex;
        try {
            parallel_merge_sort(pool, mid, last, comp, depth + 1, max_depth, token);
        } catch (...) {
            right_ex = std::current_exception();
        }

        left_task.get();

        if (right_ex)
            std::rethrow_exception(right_ex);

        if (!token.stop_requested())
            std::inplace_merge(first, mid, last, comp);
    }

} // namespace detail

// =========================================================================
// parallel_sort
// =========================================================================

/// 使用 operator< 并行排序 [first, last)。
/// 要求 RandomAccessIterator。
/// @note 此排序不稳定。
template <typename It>
auto parallel_sort(ThreadPool& pool, It first, It last) -> void
{
    parallel_sort(pool, first, last, std::less<>{});
}

/// 使用自定义比较器排序。
template <typename It, typename Comp>
auto parallel_sort(ThreadPool& pool, It first, It last, Comp comp) -> void
{
    const auto n = std::distance(first, last);
    if (n < static_cast<std::ptrdiff_t>(detail::kParallelThreshold)) {
        std::sort(first, last, comp);
        return;
    }

    const size_t max_depth = detail::log2_ceil(pool.thread_count()) + 1;
    detail::parallel_merge_sort(pool, first, last, comp, 0, max_depth);
}

/// 带取消支持的并行排序。
/// 若被取消，数组可能处于部分排序状态。
template <typename It, typename Comp>
auto parallel_sort(ThreadPool& pool, It first, It last, Comp comp, std::stop_token token) -> void
{
    if (token.stop_requested())
        throw std::runtime_error("Operation cancelled");

    const auto n = std::distance(first, last);
    if (n < static_cast<std::ptrdiff_t>(detail::kParallelThreshold)) {
        std::sort(first, last, comp);
        return;
    }

    const size_t max_depth = detail::log2_ceil(pool.thread_count()) + 1;
    detail::parallel_merge_sort(pool, first, last, comp, 0, max_depth, token);

    if (token.stop_requested())
        throw std::runtime_error("Operation cancelled");
}

} // namespace bee
