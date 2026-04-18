/**
 * @File Scan.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <atomic>
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
// parallel_inclusive_scan — 迭代器接口
// =========================================================================

/// 包含式扫描：out[i] = op(in[0], op(in[1], ... op(in[i-1], in[i])))
/// 采用三阶段 Blelloch 方法实现并行化：
///   1. 各块局部 inclusive scan（并行）
///   2. 块累加总和串行扫描得前缀偏移
///   3. 偏移修正（并行）
template <Scheduler S, std::random_access_iterator InIt, std::random_access_iterator OutIt, typename BinaryOp>
[[nodiscard]] auto parallel_inclusive_scan(S& scheduler, InIt first, InIt last, OutIt d_first, BinaryOp op) -> OutIt
{
    using ValueType = typename std::iterator_traits<InIt>::value_type;

    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0)
        return d_first;

    if (n < detail::kParallelThreshold)
        return std::inclusive_scan(first, last, d_first, op);

    auto chunks = detail::partition(n, scheduler.thread_count());

    // ── 第一阶段：各块局部 inclusive scan ──
    std::vector<ValueType>          chunk_totals(chunks.size());
    std::vector<std::exception_ptr> exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        detail::safe_post_loop(scheduler, chunks.size(), done, [&](std::size_t i) {
            return [&, i]() {
                try {
                    auto in_begin  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end    = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    std::inclusive_scan(in_begin, in_end, out_begin, op);
                    chunk_totals[i] = *std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].end - 1));
                }
                catch (...) {
                    exceptions[i] = std::current_exception();
                }
                done.count_down();
            };
        });
    }

    detail::rethrow_first(exceptions);

    // ── 第二阶段：串行计算块前缀偏移 ──
    std::vector<ValueType> chunk_offsets(chunks.size());
    for (std::size_t i = 1; i < chunks.size(); ++i) {
        chunk_offsets[i] = (i == 1) ? chunk_totals[0] : op(chunk_offsets[i - 1], chunk_totals[i - 1]);
    }

    // ── 第三阶段：偏移修正（跳过第 0 块）──
    if (chunks.size() > 1) {
        std::vector<std::exception_ptr> fixup_exceptions(chunks.size() - 1);
        std::latch                      fixup_done(static_cast<std::ptrdiff_t>(chunks.size() - 1));
        detail::safe_post_loop(scheduler, chunks.size() - 1, fixup_done, [&](std::size_t idx) {
            const std::size_t i = idx + 1;
            return [&, i]() {
                try {
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto out_end   = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    for (auto it = out_begin; it != out_end; ++it) {
                        *it = op(chunk_offsets[i], *it);
                    }
                }
                catch (...) {
                    fixup_exceptions[idx] = std::current_exception();
                }
                fixup_done.count_down();
            };
        });

        detail::rethrow_first(fixup_exceptions);
    }

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

/// 带取消支持的包含式扫描。
template <Scheduler S, std::random_access_iterator InIt, std::random_access_iterator OutIt, typename BinaryOp>
[[nodiscard]] auto parallel_inclusive_scan(S& scheduler, InIt first, InIt last, OutIt d_first, BinaryOp op, std::stop_token token) -> OutIt
{
    using ValueType = typename std::iterator_traits<InIt>::value_type;

    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0)
        return d_first;

    if (token.stop_requested())
        throw std::runtime_error("Operation cancelled");

    if (n < detail::kParallelThreshold) {
        auto      in  = first;
        auto      out = d_first;
        ValueType acc = *in;
        *out          = acc;
        ++in;
        ++out;
        for (; in != last; ++in, ++out) {
            if (token.stop_requested())
                throw std::runtime_error("Operation cancelled");
            acc  = op(acc, *in);
            *out = acc;
        }
        return out;
    }

    auto              chunks = detail::partition(n, scheduler.thread_count());
    std::atomic<bool> cancelled{false};

    // 第一阶段
    std::vector<ValueType>          chunk_totals(chunks.size());
    std::vector<std::exception_ptr> exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        detail::safe_post_loop(scheduler, chunks.size(), done, [&](std::size_t i) {
            return [&, i]() {
                if (token.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    done.count_down();
                    return;
                }
                try {
                    auto in_begin  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end    = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    std::inclusive_scan(in_begin, in_end, out_begin, op);
                    chunk_totals[i] = *std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].end - 1));
                }
                catch (...) {
                    exceptions[i] = std::current_exception();
                }
                done.count_down();
            };
        });
    }

    detail::rethrow_first(exceptions);
    if (cancelled.load(std::memory_order_acquire))
        throw std::runtime_error("Operation cancelled");

    // 第二阶段
    std::vector<ValueType> chunk_offsets(chunks.size());
    for (std::size_t i = 1; i < chunks.size(); ++i) {
        chunk_offsets[i] = (i == 1) ? chunk_totals[0] : op(chunk_offsets[i - 1], chunk_totals[i - 1]);
    }

    // 第三阶段
    if (chunks.size() > 1) {
        std::vector<std::exception_ptr> fixup_exceptions(chunks.size() - 1);
        std::latch                      fixup_done(static_cast<std::ptrdiff_t>(chunks.size() - 1));
        detail::safe_post_loop(scheduler, chunks.size() - 1, fixup_done, [&](std::size_t idx) {
            const std::size_t i = idx + 1;
            return [&, i]() {
                if (token.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    fixup_done.count_down();
                    return;
                }
                try {
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto out_end   = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    for (auto it = out_begin; it != out_end; ++it) {
                        *it = op(chunk_offsets[i], *it);
                    }
                }
                catch (...) {
                    fixup_exceptions[idx] = std::current_exception();
                }
                fixup_done.count_down();
            };
        });

        detail::rethrow_first(fixup_exceptions);
        if (cancelled.load(std::memory_order_acquire))
            throw std::runtime_error("Operation cancelled");
    }

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

// =========================================================================
// parallel_inclusive_scan — Ranges 接口
// =========================================================================

template <Scheduler S, std::ranges::random_access_range R, std::random_access_iterator OutIt, typename BinaryOp>
[[nodiscard]] auto parallel_inclusive_scan(S& scheduler, R&& range, OutIt d_first, BinaryOp op) -> OutIt
{
    return parallel_inclusive_scan(scheduler, std::ranges::begin(range), std::ranges::end(range), d_first, op);
}

template <Scheduler S, std::ranges::random_access_range R, std::random_access_iterator OutIt, typename BinaryOp>
[[nodiscard]] auto parallel_inclusive_scan(S& scheduler, R&& range, OutIt d_first, BinaryOp op, std::stop_token token) -> OutIt
{
    return parallel_inclusive_scan(scheduler, std::ranges::begin(range), std::ranges::end(range), d_first, op, token);
}

// =========================================================================
// parallel_exclusive_scan — 迭代器接口
// =========================================================================

/// 排除式扫描：out[0] = init, out[i] = op(init, in[0], ..., in[i-1])
/// 三阶段并行实现：
///   1. 各块局部归约得块总和（并行）
///   2. 块前缀偏移串行计算
///   3. 各块并行写出排除式扫描结果
template <Scheduler S, std::random_access_iterator InIt, std::random_access_iterator OutIt, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_exclusive_scan(S& scheduler, InIt first, InIt last, OutIt d_first, T init, BinaryOp op) -> OutIt
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0)
        return d_first;

    if (n < detail::kParallelThreshold)
        return std::exclusive_scan(first, last, d_first, init, op);

    auto chunks = detail::partition(n, scheduler.thread_count());

    // ── 第一阶段：各块局部归约 ──
    std::vector<T>                  chunk_sums(chunks.size());
    std::vector<std::exception_ptr> exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        detail::safe_post_loop(scheduler, chunks.size(), done, [&](std::size_t i) {
            return [&, i]() {
                try {
                    auto in_begin = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end   = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    T    acc      = *in_begin;
                    ++in_begin;
                    for (; in_begin != in_end; ++in_begin) {
                        acc = op(acc, *in_begin);
                    }
                    chunk_sums[i] = std::move(acc);
                }
                catch (...) {
                    exceptions[i] = std::current_exception();
                }
                done.count_down();
            };
        });
    }

    detail::rethrow_first(exceptions);

    // ── 第二阶段：计算排除式前缀偏移 ──
    std::vector<T> chunk_offsets(chunks.size());
    chunk_offsets[0] = init;
    for (std::size_t i = 1; i < chunks.size(); ++i) {
        chunk_offsets[i] = op(chunk_offsets[i - 1], chunk_sums[i - 1]);
    }

    // ── 第三阶段：各块并行写出排除式扫描结果 ──
    std::vector<std::exception_ptr> write_exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        detail::safe_post_loop(scheduler, chunks.size(), done, [&](std::size_t i) {
            return [&, i]() {
                try {
                    auto in_begin  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end    = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    T    running   = chunk_offsets[i];
                    for (auto in_it = in_begin; in_it != in_end; ++in_it, ++out_begin) {
                        *out_begin = running;
                        running    = op(running, *in_it);
                    }
                }
                catch (...) {
                    write_exceptions[i] = std::current_exception();
                }
                done.count_down();
            };
        });
    }

    detail::rethrow_first(write_exceptions);

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

/// 带取消支持的排除式扫描。
template <Scheduler S, std::random_access_iterator InIt, std::random_access_iterator OutIt, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_exclusive_scan(S& scheduler, InIt first, InIt last, OutIt d_first, T init, BinaryOp op, std::stop_token token)
    -> OutIt
{
    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0)
        return d_first;

    if (token.stop_requested())
        throw std::runtime_error("Operation cancelled");

    if (n < detail::kParallelThreshold) {
        T    running = init;
        auto in_it   = first;
        auto out_it  = d_first;
        for (; in_it != last; ++in_it, ++out_it) {
            if (token.stop_requested())
                throw std::runtime_error("Operation cancelled");
            *out_it = running;
            running = op(running, *in_it);
        }
        return out_it;
    }

    auto              chunks = detail::partition(n, scheduler.thread_count());
    std::atomic<bool> cancelled{false};

    // 第一阶段
    std::vector<T>                  chunk_sums(chunks.size());
    std::vector<std::exception_ptr> exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        detail::safe_post_loop(scheduler, chunks.size(), done, [&](std::size_t i) {
            return [&, i]() {
                if (token.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    done.count_down();
                    return;
                }
                try {
                    auto in_begin = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end   = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    T    acc      = *in_begin;
                    ++in_begin;
                    for (; in_begin != in_end; ++in_begin) {
                        acc = op(acc, *in_begin);
                    }
                    chunk_sums[i] = std::move(acc);
                }
                catch (...) {
                    exceptions[i] = std::current_exception();
                }
                done.count_down();
            };
        });
    }

    detail::rethrow_first(exceptions);
    if (cancelled.load(std::memory_order_acquire))
        throw std::runtime_error("Operation cancelled");

    // 第二阶段
    std::vector<T> chunk_offsets(chunks.size());
    chunk_offsets[0] = init;
    for (std::size_t i = 1; i < chunks.size(); ++i) {
        chunk_offsets[i] = op(chunk_offsets[i - 1], chunk_sums[i - 1]);
    }

    // 第三阶段
    std::vector<std::exception_ptr> write_exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        detail::safe_post_loop(scheduler, chunks.size(), done, [&](std::size_t i) {
            return [&, i]() {
                if (token.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    done.count_down();
                    return;
                }
                try {
                    auto in_begin  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end    = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    T    running   = chunk_offsets[i];
                    for (auto in_it = in_begin; in_it != in_end; ++in_it, ++out_begin) {
                        *out_begin = running;
                        running    = op(running, *in_it);
                    }
                }
                catch (...) {
                    write_exceptions[i] = std::current_exception();
                }
                done.count_down();
            };
        });
    }

    detail::rethrow_first(write_exceptions);
    if (cancelled.load(std::memory_order_acquire))
        throw std::runtime_error("Operation cancelled");

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

// =========================================================================
// parallel_exclusive_scan — Ranges 接口
// =========================================================================

template <Scheduler S, std::ranges::random_access_range R, std::random_access_iterator OutIt, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_exclusive_scan(S& scheduler, R&& range, OutIt d_first, T init, BinaryOp op) -> OutIt
{
    return parallel_exclusive_scan(scheduler, std::ranges::begin(range), std::ranges::end(range), d_first, init, op);
}

template <Scheduler S, std::ranges::random_access_range R, std::random_access_iterator OutIt, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_exclusive_scan(S& scheduler, R&& range, OutIt d_first, T init, BinaryOp op, std::stop_token token) -> OutIt
{
    return parallel_exclusive_scan(scheduler, std::ranges::begin(range), std::ranges::end(range), d_first, init, op, token);
}

} // namespace bee
