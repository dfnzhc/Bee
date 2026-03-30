/**
 * @File Block.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cuda_runtime.h>
#include <type_traits>

#include "Warp.cuh"

namespace bee::cuda
{

// =========================================================
// 基础索引工具
// =========================================================

/**
 * @brief 返回当前线程在 block 中的 3D 线性下标。
 *
 * 对于算子 kernel，很多共享内存访问和分工逻辑都基于线性 tid。
 */
__device__ __forceinline__ int linear_tid()
{
    // clang-format off
    return static_cast<int>(threadIdx.x) +
           static_cast<int>(blockDim.x) * (static_cast<int>(threadIdx.y) + 
           static_cast<int>(blockDim.y) * static_cast<int>(threadIdx.z));
    // clang-format on
}

/**
 * @brief 是否为 block 内第一个线程。
 */
__device__ __forceinline__ bool is_first_thread()
{
    return linear_tid() == 0;
}

/**
 * @brief 计算当前线程属于 block 内第几个 warp。
 *
 * 线程按 x->y->z 线性化后再除以 warp 大小。
 */
__device__ __forceinline__ int warp_id_in_block()
{
    return linear_tid() / kWarpSize;
}

// =========================================================
// Block Reduce / All-Reduce
// =========================================================

template <typename T, int BlockSize>
__device__ __forceinline__ T reduce_sum(T value)
{
    static_assert(BlockSize > 0, "BlockSize must be positive");
    static_assert(std::is_arithmetic_v<T>, "reduce_sum requires arithmetic type");

    constexpr int kWarpCount = (BlockSize + kWarpSize - 1) / kWarpSize;
    __shared__ T warp_results[kWarpCount];

    const int lane = lane_id();
    const int wid  = warp_id_in_block();

    const unsigned lane_mask = active_mask();
    T local                  = reduce_sum(value, lane_mask);
    if (lane == 0) {
        warp_results[wid] = local;
    }
    __syncthreads();

    T block_value = static_cast<T>(0);
    if (wid == 0) {
        block_value = lane < kWarpCount ? warp_results[lane] : static_cast<T>(0);
        block_value = reduce_sum(block_value, lane_mask);
    }

    __shared__ T final_value;
    if (is_first_thread()) {
        final_value = block_value;
    }
    __syncthreads();

    return final_value;
}

template <typename T, int BlockSize>
__device__ __forceinline__ T reduce_max(T value)
{
    static_assert(BlockSize > 0, "BlockSize must be positive");
    static_assert(std::is_arithmetic_v<T>, "reduce_max requires arithmetic type");

    constexpr int kWarpCount = (BlockSize + kWarpSize - 1) / kWarpSize;
    __shared__ T warp_results[kWarpCount];

    const int lane = lane_id();
    const int wid  = warp_id_in_block();

    const unsigned lane_mask = active_mask();
    T local                  = reduce_max(value, lane_mask);
    if (lane == 0) {
        warp_results[wid] = local;
    }
    __syncthreads();

    T block_value = value;
    if (wid == 0) {
        // 这里复用 warp_results[0] 作为非参与 lane 的填充值，避免在 device 侧依赖 numeric_limits。
        block_value = lane < kWarpCount ? warp_results[lane] : warp_results[0];
        block_value = reduce_max(block_value, lane_mask);
    }

    __shared__ T final_value;
    if (is_first_thread()) {
        final_value = block_value;
    }
    __syncthreads();

    return final_value;
}

template <typename T, int BlockSize>
__device__ __forceinline__ T reduce_min(T value)
{
    static_assert(BlockSize > 0, "BlockSize must be positive");
    static_assert(std::is_arithmetic_v<T>, "reduce_min requires arithmetic type");

    constexpr int kWarpCount = (BlockSize + kWarpSize - 1) / kWarpSize;
    __shared__ T warp_results[kWarpCount];

    const int lane = lane_id();
    const int wid  = warp_id_in_block();

    const unsigned lane_mask = active_mask();
    T local                  = reduce_min(value, lane_mask);
    if (lane == 0) {
        warp_results[wid] = local;
    }
    __syncthreads();

    T block_value = value;
    if (wid == 0) {
        // 这里复用 warp_results[0] 作为非参与 lane 的填充值，避免在 device 侧依赖 numeric_limits。
        block_value = lane < kWarpCount ? warp_results[lane] : warp_results[0];
        block_value = reduce_min(block_value, lane_mask);
    }

    __shared__ T final_value;
    if (is_first_thread()) {
        final_value = block_value;
    }
    __syncthreads();

    return final_value;
}

/**
 * @brief Block 级乘积归约。
 *
 * 常用于 small-tile 的缩放因子累积、概率连乘等场景。
 */
template <typename T, int BlockSize>
__device__ __forceinline__ T reduce_prod(T value)
{
    static_assert(BlockSize > 0, "BlockSize must be positive");
    static_assert(std::is_arithmetic_v<T>, "reduce_prod requires arithmetic type");

    constexpr int kWarpCount = (BlockSize + kWarpSize - 1) / kWarpSize;
    __shared__ T warp_results[kWarpCount];

    const int lane = lane_id();
    const int wid  = warp_id_in_block();

    const unsigned lane_mask = active_mask();
    T local                  = reduce_prod(value, lane_mask);
    if (lane == 0) {
        warp_results[wid] = local;
    }
    __syncthreads();

    T block_value = static_cast<T>(1);
    if (wid == 0) {
        block_value = lane < kWarpCount ? warp_results[lane] : static_cast<T>(1);
        block_value = reduce_prod(block_value, lane_mask);
    }

    __shared__ T final_value;
    if (is_first_thread()) {
        final_value = block_value;
    }
    __syncthreads();

    return final_value;
}

// =========================================================
// Block Broadcast / Vote / Count
// =========================================================

/**
 * @brief 从 block 内任意线程广播一个值到所有线程。
 *
 * @param value    每个线程本地值。
 * @param src_tid  广播源线程（0 <= src_tid < BlockSize）。
 */
template <typename T, int BlockSize>
__device__ __forceinline__ T broadcast(T value, int src_tid)
{
    static_assert(BlockSize > 0, "BlockSize must be positive");

    __shared__ T shared_value;
    const int tid = linear_tid();
    if (tid == src_tid) {
        shared_value = value;
    }
    __syncthreads();
    return shared_value;
}

/**
 * @brief 统计 block 内 predicate=true 的线程数量。
 */
template <int BlockSize>
__device__ __forceinline__ int count_if(bool predicate)
{
    static_assert(BlockSize > 0, "BlockSize must be positive");
    return reduce_sum<int, BlockSize>(predicate ? 1 : 0);
}

/**
 * @brief block 级 any：是否至少一个线程满足条件。
 */
template <int BlockSize>
__device__ __forceinline__ bool any(bool predicate)
{
    return count_if<BlockSize>(predicate) > 0;
}

/**
 * @brief block 级 all：是否所有线程都满足条件。
 */
template <int BlockSize>
__device__ __forceinline__ bool all(bool predicate)
{
    return count_if<BlockSize>(predicate) == BlockSize;
}

// =========================================================
// Block Scan 与 Stream-Compaction 辅助
// =========================================================

/**
 * @brief Block 级 exclusive scan（高效分层实现）。
 *
 * 实现思路：
 * 1) 每个 warp 内先做前缀和；
 * 2) 把每个 warp 的总和写入 shared memory；
 * 3) 由 warp0 对 warp 总和再做一次 scan，得到每个 warp 的偏移；
 * 4) 局部前缀和 + warp 偏移得到最终结果。
 *
 * 相比朴素 Hillis-Steele 的全 block O(BlockSize*logBlockSize) 反复同步，
 * 该版本减少了 shared memory 往返和同步次数，适合在 kernel 热路径使用。
 */
template <typename T, int BlockSize>
__device__ __forceinline__ T exclusive_scan(T value)
{
    static_assert(BlockSize > 0, "BlockSize must be positive");
    static_assert(std::is_arithmetic_v<T>, "exclusive_scan requires arithmetic type");

    constexpr int kWarpCount = (BlockSize + kWarpSize - 1) / kWarpSize;
    __shared__ T warp_exclusive_offsets[kWarpCount];

    const int tid  = linear_tid();
    const int lane = lane_id();
    const int wid  = warp_id_in_block();

    // Step 1: each warp computes its local inclusive prefix.
    const T warp_inclusive = inclusive_prefix_sum(value);

    // Step 2: write each warp total to shared memory.
    if (lane == (kWarpSize - 1) || tid == (BlockSize - 1)) {
        warp_exclusive_offsets[wid] = warp_inclusive;
    }
    __syncthreads();

    // Step 3: warp 0 scans warp totals and converts to exclusive warp offsets.
    if (wid == 0) {
        T warp_total                 = lane < kWarpCount ? warp_exclusive_offsets[lane] : static_cast<T>(0);
        const T warp_inclusive_total = inclusive_prefix_sum(warp_total);
        if (lane < kWarpCount) {
            warp_exclusive_offsets[lane] = warp_inclusive_total - warp_total;
        }
    }
    __syncthreads();

    const T warp_offset     = warp_exclusive_offsets[wid];
    const T local_exclusive = warp_inclusive - value;
    return warp_offset + local_exclusive;
}

/**
 * @brief Block 级 inclusive scan。
 */
template <typename T, int BlockSize>
__device__ __forceinline__ T inclusive_scan(T value)
{
    return exclusive_scan<T, BlockSize>(value) + value;
}

/**
 * @brief 计算当前线程在 block 紧凑写出中的下标。
 *
 * 该函数对 true/false 线程都返回“之前有多少个 true”，
 * 调用方通常配合 predicate 判断是否写出。
 */
template <int BlockSize>
__device__ __forceinline__ int compact_index(bool predicate)
{
    return exclusive_scan<int, BlockSize>(predicate ? 1 : 0);
}

/**
 * @brief 返回 block 级紧凑写出总数（即 predicate=true 的总线程数）。
 */
template <int BlockSize>
__device__ __forceinline__ int compact_count(bool predicate)
{
    return count_if<BlockSize>(predicate);
}

} // namespace bee::cuda
