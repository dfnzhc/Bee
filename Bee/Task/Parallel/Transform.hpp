/**
 * @File Transform.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/12
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <iterator>
#include <stop_token>

#include "Task/Parallel/Partitioner.hpp"

namespace bee
{

/// 并行将 [first, last) 变换到 [d_first, ...)。
/// 返回输出区间的尾后迭代器。
template <typename InIt, typename OutIt, typename Fn>
[[nodiscard]] auto parallel_transform(ThreadPool& pool, InIt first, InIt last, OutIt d_first, Fn fn) -> OutIt
{
    const auto n = static_cast<size_t>(std::distance(first, last));
    if (n < detail::kParallelThreshold) {
        return std::transform(first, last, d_first, fn);
    }

    detail::execute_chunks(pool, n, [first, d_first, &fn](size_t begin, size_t end) {
        auto in  = std::next(first, static_cast<std::ptrdiff_t>(begin));
        auto out = std::next(d_first, static_cast<std::ptrdiff_t>(begin));
        for (size_t i = begin; i < end; ++i, ++in, ++out) {
            *out = fn(*in);
        }
    });

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

/// 带取消支持的并行变换。
template <typename InIt, typename OutIt, typename Fn>
[[nodiscard]] auto parallel_transform(ThreadPool& pool, InIt first, InIt last, OutIt d_first, Fn fn, std::stop_token token) -> OutIt
{
    const auto n = static_cast<size_t>(std::distance(first, last));
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
            pool,
            n,
            [first, d_first, &fn, token](size_t begin, size_t end) {
                auto in  = std::next(first, static_cast<std::ptrdiff_t>(begin));
                auto out = std::next(d_first, static_cast<std::ptrdiff_t>(begin));
                for (size_t i = begin; i < end; ++i, ++in, ++out) {
                    if (token.stop_requested())
                        return;
                    *out = fn(*in);
                }
            },
            token
    );

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

} // namespace bee
