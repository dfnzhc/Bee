/**
 * @File ForEach.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <stop_token>
#include <stdexcept>

#include "Task/Parallel/Partitioner.hpp"

namespace bee
{

// =========================================================================
// parallel_for_each — 迭代器接口
// =========================================================================

/// 并行地对 [first, last) 中每个元素应用 fn。
/// 区间大小低于 kParallelThreshold 时退化为串行执行。
template <Scheduler S, std::random_access_iterator It, typename Fn>
auto parallel_for_each(S& scheduler, It first, It last, Fn fn) -> void
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n < detail::kParallelThreshold) {
        std::for_each(first, last, fn);
        return;
    }

    detail::execute_chunks(scheduler, n, [first, &fn](std::size_t begin, std::size_t end) {
        auto it = std::next(first, static_cast<std::ptrdiff_t>(begin));
        for (std::size_t i = begin; i < end; ++i, ++it) {
            fn(*it);
        }
    });
}

/// 带取消支持的并行遍历。
template <Scheduler S, std::random_access_iterator It, typename Fn>
auto parallel_for_each(S& scheduler, It first, It last, Fn fn, std::stop_token token) -> void
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n < detail::kParallelThreshold) {
        for (auto it = first; it != last; ++it) {
            if (token.stop_requested())
                throw std::runtime_error("Operation cancelled");
            fn(*it);
        }
        return;
    }

    detail::execute_chunks(
        scheduler,
        n,
        [first, &fn, token](std::size_t begin, std::size_t end) {
            auto it = std::next(first, static_cast<std::ptrdiff_t>(begin));
            for (std::size_t i = begin; i < end; ++i, ++it) {
                if (token.stop_requested())
                    return;
                fn(*it);
            }
        },
        token);
}

// =========================================================================
// parallel_for_each — Ranges 接口
// =========================================================================

/// Ranges 版本：并行对 range 中每个元素应用 fn。
template <Scheduler S, std::ranges::random_access_range R, typename Fn>
auto parallel_for_each(S& scheduler, R&& range, Fn fn) -> void
{
    parallel_for_each(scheduler, std::ranges::begin(range), std::ranges::end(range), fn);
}

/// Ranges 版本 + 取消支持。
template <Scheduler S, std::ranges::random_access_range R, typename Fn>
auto parallel_for_each(S& scheduler, R&& range, Fn fn, std::stop_token token) -> void
{
    parallel_for_each(scheduler, std::ranges::begin(range), std::ranges::end(range), fn, token);
}

} // namespace bee
