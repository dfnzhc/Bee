/**
 * @File Scan.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/12
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <iterator>
#include <latch>
#include <numeric>
#include <stop_token>
#include <stdexcept>
#include <vector>

#include "Task/Parallel/Partitioner.hpp"

namespace bee
{

// =========================================================================
// parallel_inclusive_scan
// =========================================================================

/// 包含式扫描：out[i] = op(in[0], op(in[1], ... op(in[i-1], in[i])))
/// 采用三阶段 Blelloch 方法实现并行化。
template <typename InIt, typename OutIt, typename BinaryOp>
[[nodiscard]] auto parallel_inclusive_scan(ThreadPool& pool, InIt first, InIt last, OutIt d_first, BinaryOp op) -> OutIt
{
    using ValueType = typename std::iterator_traits<InIt>::value_type;

    const auto n = static_cast<size_t>(std::distance(first, last));
    if (n == 0)
        return d_first;

    if (n < detail::kParallelThreshold)
        return std::inclusive_scan(first, last, d_first, op);

    auto chunks = detail::partition(n, pool.thread_count());

    // 第一阶段：各块局部 inclusive scan（并行）
    std::vector<ValueType> chunk_totals(chunks.size());
    std::vector<std::exception_ptr> exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        for (size_t i = 0; i < chunks.size(); ++i) {
            pool.post([&, i]() {
                try {
                    auto in_begin  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end    = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    std::inclusive_scan(in_begin, in_end, out_begin, op);
                    // 块输出的最后一个元素即为该块的累加总和。
                    chunk_totals[i] = *std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].end - 1));
                } catch (...) {
                    exceptions[i] = std::current_exception();
                }
                done.count_down();
            });
        }
        done.wait();
    }

    for (auto& ex : exceptions) {
        if (ex)
            std::rethrow_exception(ex);
    }

    // 第二阶段：对块累加总和做串行扫描以获取前缀偏移。
    // chunk_offsets[0] 未使用——第 0 块无需偏移调整。
    // 对于 i >= 1：offset = op(totals[0], totals[1], ..., totals[i-1])
    std::vector<ValueType> chunk_offsets(chunks.size());
    for (size_t i = 1; i < chunks.size(); ++i) {
        chunk_offsets[i] = (i == 1) ? chunk_totals[0] : op(chunk_offsets[i - 1], chunk_totals[i - 1]);
    }

    // 第三阶段：修正遍历（并行）——为每个块加上偏移（跳过第一块）
    if (chunks.size() > 1) {
        std::vector<std::exception_ptr> fixup_exceptions(chunks.size() - 1);
        std::latch fixup_done(static_cast<std::ptrdiff_t>(chunks.size() - 1));
        for (size_t i = 1; i < chunks.size(); ++i) {
            pool.post([&, i]() {
                try {
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto out_end   = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    for (auto it = out_begin; it != out_end; ++it) {
                        *it = op(chunk_offsets[i], *it);
                    }
                } catch (...) {
                    fixup_exceptions[i - 1] = std::current_exception();
                }
                fixup_done.count_down();
            });
        }
        fixup_done.wait();

        for (auto& ex : fixup_exceptions) {
            if (ex)
                std::rethrow_exception(ex);
        }
    }

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

/// 带取消支持的包含式扫描。
/// @note 取消时，输出区间可能包含部分结果。
template <typename InIt, typename OutIt, typename BinaryOp>
[[nodiscard]] auto parallel_inclusive_scan(ThreadPool& pool, InIt first, InIt last, OutIt d_first, BinaryOp op, std::stop_token token) -> OutIt
{
    using ValueType = typename std::iterator_traits<InIt>::value_type;

    const auto n = static_cast<size_t>(std::distance(first, last));
    if (n == 0)
        return d_first;

    if (token.stop_requested())
        throw std::runtime_error("Operation cancelled");

    if (n < detail::kParallelThreshold) {
        auto in       = first;
        auto out      = d_first;
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

    auto chunks = detail::partition(n, pool.thread_count());
    std::vector<ValueType> chunk_totals(chunks.size());
    std::vector<std::exception_ptr> exceptions(chunks.size());
    std::atomic<bool> cancelled{false};

    // 第一阶段
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        for (size_t i = 0; i < chunks.size(); ++i) {
            pool.post([&, i]() {
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
                } catch (...) {
                    exceptions[i] = std::current_exception();
                }
                done.count_down();
            });
        }
        done.wait();
    }

    for (auto& ex : exceptions) {
        if (ex)
            std::rethrow_exception(ex);
    }
    if (cancelled.load(std::memory_order_acquire))
        throw std::runtime_error("Operation cancelled");

    // 第二阶段（串行）
    std::vector<ValueType> chunk_offsets(chunks.size());
    for (size_t i = 1; i < chunks.size(); ++i) {
        chunk_offsets[i] = (i == 1) ? chunk_totals[0] : op(chunk_offsets[i - 1], chunk_totals[i - 1]);
    }

    // 第三阶段
    if (chunks.size() > 1) {
        std::vector<std::exception_ptr> fixup_exceptions(chunks.size() - 1);
        std::latch fixup_done(static_cast<std::ptrdiff_t>(chunks.size() - 1));
        for (size_t i = 1; i < chunks.size(); ++i) {
            pool.post([&, i]() {
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
                } catch (...) {
                    fixup_exceptions[i - 1] = std::current_exception();
                }
                fixup_done.count_down();
            });
        }
        fixup_done.wait();

        for (auto& ex : fixup_exceptions) {
            if (ex)
                std::rethrow_exception(ex);
        }
        if (cancelled.load(std::memory_order_acquire))
            throw std::runtime_error("Operation cancelled");
    }

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

// =========================================================================
// parallel_exclusive_scan
// =========================================================================

/// 排除式扫描：out[0] = init, out[i] = op(init, in[0], ..., in[i-1])
template <typename InIt, typename OutIt, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_exclusive_scan(ThreadPool& pool, InIt first, InIt last, OutIt d_first, T init, BinaryOp op) -> OutIt
{
    const auto n = static_cast<size_t>(std::distance(first, last));
    if (n == 0)
        return d_first;

    if (n < detail::kParallelThreshold)
        return std::exclusive_scan(first, last, d_first, init, op);

    auto chunks = detail::partition(n, pool.thread_count());

    // 第一阶段：各块局部归约以计算块累加总和
    std::vector<T> chunk_sums(chunks.size());
    std::vector<std::exception_ptr> exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        for (size_t i = 0; i < chunks.size(); ++i) {
            pool.post([&, i]() {
                try {
                    auto in_begin = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end   = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    T acc         = *in_begin;
                    ++in_begin;
                    for (; in_begin != in_end; ++in_begin) {
                        acc = op(acc, *in_begin);
                    }
                    chunk_sums[i] = std::move(acc);
                } catch (...) {
                    exceptions[i] = std::current_exception();
                }
                done.count_down();
            });
        }
        done.wait();
    }

    for (auto& ex : exceptions) {
        if (ex)
            std::rethrow_exception(ex);
    }

    // 第二阶段：计算每个块的排除式前缀偏移
    std::vector<T> chunk_offsets(chunks.size());
    chunk_offsets[0] = init;
    for (size_t i = 1; i < chunks.size(); ++i) {
        chunk_offsets[i] = op(chunk_offsets[i - 1], chunk_sums[i - 1]);
    }

    // 第三阶段：各块并行写出排除式扫描结果
    std::vector<std::exception_ptr> write_exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        for (size_t i = 0; i < chunks.size(); ++i) {
            pool.post([&, i]() {
                try {
                    auto in_begin  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end    = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    T running      = chunk_offsets[i];
                    for (auto in_it = in_begin; in_it != in_end; ++in_it, ++out_begin) {
                        *out_begin = running;
                        running    = op(running, *in_it);
                    }
                } catch (...) {
                    write_exceptions[i] = std::current_exception();
                }
                done.count_down();
            });
        }
        done.wait();
    }

    for (auto& ex : write_exceptions) {
        if (ex)
            std::rethrow_exception(ex);
    }

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

/// 带取消支持的排除式扫描。
/// @note 取消时，输出区间可能包含部分结果。
template <typename InIt, typename OutIt, typename T, typename BinaryOp>
[[nodiscard]] auto parallel_exclusive_scan(ThreadPool& pool, InIt first, InIt last, OutIt d_first, T init, BinaryOp op,
                                           std::stop_token token) -> OutIt
{
    const auto n = static_cast<size_t>(std::distance(first, last));
    if (n == 0)
        return d_first;

    if (token.stop_requested())
        throw std::runtime_error("Operation cancelled");

    if (n < detail::kParallelThreshold) {
        T running   = init;
        auto in_it  = first;
        auto out_it = d_first;
        for (; in_it != last; ++in_it, ++out_it) {
            if (token.stop_requested())
                throw std::runtime_error("Operation cancelled");
            *out_it = running;
            running = op(running, *in_it);
        }
        return out_it;
    }

    auto chunks = detail::partition(n, pool.thread_count());
    std::atomic<bool> cancelled{false};

    // 第一阶段：各块局部归约
    std::vector<T> chunk_sums(chunks.size());
    std::vector<std::exception_ptr> exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        for (size_t i = 0; i < chunks.size(); ++i) {
            pool.post([&, i]() {
                if (token.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    done.count_down();
                    return;
                }
                try {
                    auto in_begin = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end   = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    T acc         = *in_begin;
                    ++in_begin;
                    for (; in_begin != in_end; ++in_begin) {
                        acc = op(acc, *in_begin);
                    }
                    chunk_sums[i] = std::move(acc);
                } catch (...) {
                    exceptions[i] = std::current_exception();
                }
                done.count_down();
            });
        }
        done.wait();
    }

    for (auto& ex : exceptions) {
        if (ex)
            std::rethrow_exception(ex);
    }
    if (cancelled.load(std::memory_order_acquire))
        throw std::runtime_error("Operation cancelled");

    // 第二阶段：串行计算前缀偏移
    std::vector<T> chunk_offsets(chunks.size());
    chunk_offsets[0] = init;
    for (size_t i = 1; i < chunks.size(); ++i) {
        chunk_offsets[i] = op(chunk_offsets[i - 1], chunk_sums[i - 1]);
    }

    // 第三阶段：并行写出
    std::vector<std::exception_ptr> write_exceptions(chunks.size());
    {
        std::latch done(static_cast<std::ptrdiff_t>(chunks.size()));
        for (size_t i = 0; i < chunks.size(); ++i) {
            pool.post([&, i]() {
                if (token.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    done.count_down();
                    return;
                }
                try {
                    auto in_begin  = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    auto in_end    = std::next(first, static_cast<std::ptrdiff_t>(chunks[i].end));
                    auto out_begin = std::next(d_first, static_cast<std::ptrdiff_t>(chunks[i].begin));
                    T running      = chunk_offsets[i];
                    for (auto in_it = in_begin; in_it != in_end; ++in_it, ++out_begin) {
                        *out_begin = running;
                        running    = op(running, *in_it);
                    }
                } catch (...) {
                    write_exceptions[i] = std::current_exception();
                }
                done.count_down();
            });
        }
        done.wait();
    }

    for (auto& ex : write_exceptions) {
        if (ex)
            std::rethrow_exception(ex);
    }
    if (cancelled.load(std::memory_order_acquire))
        throw std::runtime_error("Operation cancelled");

    return std::next(d_first, static_cast<std::ptrdiff_t>(n));
}

} // namespace bee
