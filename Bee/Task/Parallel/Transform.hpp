/**
 * @File Transform.hpp
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
// parallel_transform — 迭代器接口
// =========================================================================

/// 并行将 [first, last) 变换到 [d_first, ...)。
/// 返回输出区间的尾后迭代器。
template <Scheduler S, std::random_access_iterator InIt, std::random_access_iterator OutIt, typename Fn>
[[nodiscard]] auto parallel_transform(S& scheduler, InIt first, InIt last, OutIt d_first, Fn fn) -> OutIt
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n < detail::kParallelThreshold) {
        return std::transform(first, last, d_first, fn);
    }

    detail::execute_chunks(scheduler, n, [first, d_first, &fn](std::size_t begin, std::size_t end) {
        auto in  = std::next(first, static_cast<std::ptrdiff_t>(begin));
        auto out = std::next(d_first, static_cast<std::ptrdiff_t>(begin));
        for (std::size_t i = begin; i < end; ++i, ++in, ++out) {
            *out = fn(*in);
        }
    });

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

/// 带取消支持的并行变换。
template <Scheduler S, std::random_access_iterator InIt, std::random_access_iterator OutIt, typename Fn>
[[nodiscard]] auto parallel_transform(S& scheduler, InIt first, InIt last, OutIt d_first, Fn fn, std::stop_token token) -> OutIt
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n < detail::kParallelThreshold) {
        auto in  = first;
        auto out = d_first;
        for (; in != last; ++in, ++out) {
            if (token.stop_requested())
                throw std::runtime_error("Operation cancelled");
            *out = fn(*in);
        }
        return out;
    }

    detail::execute_chunks(
        scheduler,
        n,
        [first, d_first, &fn, token](std::size_t begin, std::size_t end) {
            auto in  = std::next(first, static_cast<std::ptrdiff_t>(begin));
            auto out = std::next(d_first, static_cast<std::ptrdiff_t>(begin));
            for (std::size_t i = begin; i < end; ++i, ++in, ++out) {
                if (token.stop_requested())
                    return;
                *out = fn(*in);
            }
        },
        token
    );

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

// =========================================================================
// parallel_transform — Ranges 接口
// =========================================================================

/// Ranges 版本：并行变换 range → [d_first, ...)。
template <Scheduler S, std::ranges::random_access_range R, std::random_access_iterator OutIt, typename Fn>
[[nodiscard]] auto parallel_transform(S& scheduler, R&& range, OutIt d_first, Fn fn) -> OutIt
{
    return parallel_transform(scheduler, std::ranges::begin(range), std::ranges::end(range), d_first, fn);
}

/// Ranges 版本 + 取消支持。
template <Scheduler S, std::ranges::random_access_range R, std::random_access_iterator OutIt, typename Fn>
[[nodiscard]] auto parallel_transform(S& scheduler, R&& range, OutIt d_first, Fn fn, std::stop_token token) -> OutIt
{
    return parallel_transform(scheduler, std::ranges::begin(range), std::ranges::end(range), d_first, fn, token);
}

} // namespace bee
