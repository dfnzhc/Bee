/**
 * @File Partitioner.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/12
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <latch>
#include <stop_token>
#include <stdexcept>
#include <vector>

#include "Concurrency/Thread/ThreadPool.hpp"

namespace bee::detail
{

// =========================================================================
// ChunkRange — 基于索引的子区间 [begin, end)
// =========================================================================

struct ChunkRange
{
    size_t begin;
    size_t end;
};

// =========================================================================
// partition — 将 [0, total) 均匀分块
// =========================================================================

/// 将 [0, total) 切分为适合 num_workers 个线程的块。
/// 使用 4 倍过量分配以实现负载均衡。
[[nodiscard]] inline auto partition(size_t total, size_t num_workers) -> std::vector<ChunkRange>
{
    if (total == 0 || num_workers == 0)
        return {};

    const size_t num_chunks = std::min(total, num_workers * 4);
    const size_t base_size  = total / num_chunks;
    const size_t remainder  = total % num_chunks;

    std::vector<ChunkRange> chunks;
    chunks.reserve(num_chunks);

    size_t offset = 0;
    for (size_t i = 0; i < num_chunks; ++i) {
        const size_t size = base_size + (i < remainder ? 1 : 0);
        chunks.push_back({offset, offset + size});
        offset += size;
    }

    return chunks;
}

// =========================================================================
// 常量
// =========================================================================

/// 低于此阈值时，算法退化为串行执行。
constexpr size_t kParallelThreshold = 4096;

// =========================================================================
// execute_chunks — 带异常/取消处理的通用并行分块执行
// =========================================================================

/// 在线程池中并行执行 chunk_fn(begin, end)。
/// 阻塞直到所有块完成。重新抛出第一个异常。
/// 若被取消且无异常，则抛出 std::runtime_error。
template <typename ChunkFn>
auto execute_chunks(ThreadPool& pool, size_t total, ChunkFn&& chunk_fn, std::stop_token token = {}) -> void
{
    auto chunks = partition(total, pool.thread_count());
    if (chunks.empty())
        return;

    std::vector<std::exception_ptr> exceptions(chunks.size());
    std::atomic<bool>               cancelled{false};
    std::latch                      done(static_cast<std::ptrdiff_t>(chunks.size()));

    for (size_t i = 0; i < chunks.size(); ++i) {
        pool.post([&, i]() {
            if (token.stop_requested()) {
                cancelled.store(true, std::memory_order_relaxed);
                done.count_down();
                return;
            }
            try {
                chunk_fn(chunks[i].begin, chunks[i].end);
            } catch (...) {
                exceptions[i] = std::current_exception();
            }
            done.count_down();
        });
    }

    done.wait();

    for (auto& ex : exceptions) {
        if (ex)
            std::rethrow_exception(ex);
    }

    if (cancelled.load(std::memory_order_acquire))
        throw std::runtime_error("Operation cancelled");
}

} // namespace bee::detail
