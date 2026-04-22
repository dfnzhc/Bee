/**
 * @File ParallelFor.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 轻量级 fork-join 并行循环原语。
 *
 *  设计目标：
 *    1. 零依赖（仅依赖 Base::Parallel::ThreadPool），供高性能数值路径使用；
 *    2. 主调用线程参与计算，避免单次派发带来的额外上下文切换开销；
 *    3. 按 grain（最小粒度）拆分，区间过小时退化为串行；
 *    4. 捕获并重抛首个异常，保证与串行等价的可观察语义。
 *
 *  语义：
 *    parallel_for(begin, end, grain, fn) —— fn 接受 [lo, hi) 子区间；
 *    parallel_for_each(begin, end, grain, fn) —— fn 接受单个索引 i。
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <exception>
#include <latch>
#include <mutex>
#include <utility>

#include "Base/Parallel/ThreadPool.hpp"

namespace bee::parallel
{

namespace detail
{

    inline std::size_t compute_chunks(std::size_t n, std::size_t grain, std::size_t max_workers) noexcept
    {
        if (grain == 0)
            grain = 1;
        const std::size_t by_grain = (n + grain - 1) / grain;
        return std::min<std::size_t>(std::max<std::size_t>(by_grain, 1), std::max<std::size_t>(max_workers, 1));
    }

} // namespace detail

/// 对区间 [begin, end) 进行 fork-join 并行：每个 worker 处理一个 [lo, hi) 子区间。
/// grain 控制最小分块粒度，区间元素数不足 2*grain 时直接串行。
template <class Fn>
void parallel_for(std::size_t begin, std::size_t end, std::size_t grain, Fn&& fn)
{
    if (begin >= end)
        return;

    const std::size_t n = end - begin;
    if (grain == 0)
        grain = 1;

    auto&             pool    = ThreadPool::instance();
    const std::size_t workers = pool.size() + 1; // 含主线程
    if (n < grain * 2 || workers <= 1) {
        fn(begin, end);
        return;
    }

    const std::size_t chunks = detail::compute_chunks(n, grain, workers);
    if (chunks <= 1) {
        fn(begin, end);
        return;
    }

    const std::size_t per = (n + chunks - 1) / chunks;

    std::latch         done(static_cast<std::ptrdiff_t>(chunks - 1));
    std::exception_ptr first_err;
    std::mutex         err_mu;

    auto run_chunk = [&](std::size_t lo, std::size_t hi) noexcept {
        try {
            fn(lo, hi);
        } catch (...) {
            std::lock_guard lk(err_mu);
            if (!first_err)
                first_err = std::current_exception();
        }
    };

    for (std::size_t c = 0; c < chunks - 1; ++c) {
        const std::size_t lo = begin + c * per;
        const std::size_t hi = std::min(end, lo + per);
        pool.submit([lo, hi, &run_chunk, &done]() {
            run_chunk(lo, hi);
            done.count_down();
        });
    }

    // 主线程处理最后一块，减少一次上下文切换。
    {
        const std::size_t lo = begin + (chunks - 1) * per;
        run_chunk(lo, end);
    }

    done.wait();

    if (first_err)
        std::rethrow_exception(first_err);
}

/// per-index 版本：fn(i)。内部做 chunk 循环，避免每个元素都做调度开销。
template <class Fn>
void parallel_for_each(std::size_t begin, std::size_t end, std::size_t grain, Fn&& fn)
{
    parallel_for(begin, end, grain, [&fn](std::size_t lo, std::size_t hi) {
        for (std::size_t i = lo; i < hi; ++i)
            fn(i);
    });
}

} // namespace bee::parallel
