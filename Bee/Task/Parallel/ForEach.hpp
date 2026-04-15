/**
 * @File ForEach.hpp
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

/// 并行地对 [first, last) 中每个元素应用 fn。
/// 区间大小低于 kParallelThreshold 时退化为串行执行。
template <typename It, typename Fn>
auto parallel_for_each(ThreadPool& pool, It first, It last, Fn fn) -> void
{
    const auto n = static_cast<size_t>(std::distance(first, last));
    if (n < detail::kParallelThreshold) {
        std::for_each(first, last, fn);
        return;
    }

    detail::execute_chunks(pool, n, [first, &fn](size_t begin, size_t end) {
        auto it = std::next(first, static_cast<std::ptrdiff_t>(begin));
        for (size_t i = begin; i < end; ++i, ++it) {
            fn(*it);
        }
    });
}

/// 带取消支持的并行遍历。
/// 在每个块内的元素间检查 stop_token。
template <typename It, typename Fn>
auto parallel_for_each(ThreadPool& pool, It first, It last, Fn fn, std::stop_token token) -> void
{
    const auto n = static_cast<size_t>(std::distance(first, last));
    if (n < detail::kParallelThreshold) {
        for (auto it = first; it != last; ++it) {
            if (token.stop_requested())
                throw std::runtime_error("Operation cancelled");
            fn(*it);
        }
        return;
    }

    detail::execute_chunks(
        pool,
        n,
        [first, &fn, token](size_t begin, size_t end) {
            auto it = std::next(first, static_cast<std::ptrdiff_t>(begin));
            for (size_t i = begin; i < end; ++i, ++it) {
                if (token.stop_requested())
                    return;
                fn(*it);
            }
        },
        token
    );
}

} // namespace bee
