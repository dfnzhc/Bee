/**
 * @File Warp.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cuda_runtime.h>
#include <type_traits>

namespace bee::cuda
{

// =========================================================
// 基础信息与 lane 掩码工具
// =========================================================

// CUDA 当前硬件 warp 大小固定为 32，这里集中定义便于后续统一引用。
constexpr int kWarpSize = 32;
// 全掩码，表示 warp 中 32 个 lane 都参与同步/通信。
constexpr unsigned kFullMask = 0xffffffffu;

/**
 * @brief 返回当前执行路径上的活跃线程掩码（active mask）。
 *
 * 掩码中第 i 位为 1 表示 lane i 当前参与执行。
 * 在分支路径下做 warp 通信时，建议显式传入该掩码，避免与非活跃线程通信导致未定义行为。
 */
__device__ __forceinline__ unsigned active_mask()
{
    return __activemask();
}

/**
 * @brief 获取线程在当前 warp 内的 lane id（范围 [0, 31]）。
 *
 * 典型用途：
 * 1) 判断当前线程是否为 warp 的 leader（lane==0）；
 * 2) 在 warp 内做分工（例如 lane<16 做一类工作）。
 */
__device__ __forceinline__ int lane_id()
{
    int lane;
    asm volatile("mov.u32 %0, %%laneid;" : "=r"(lane));
    return lane;
}

/**
 * @brief 返回本 lane 左侧的掩码（不含自身）。
 */
__device__ __forceinline__ unsigned lane_mask_lt()
{
    unsigned v;
    asm volatile("mov.u32 %0, %%lanemask_lt;" : "=r"(v));
    return v;
}

/**
 * @brief 返回本 lane 左侧掩码（含自身）。
 */
__device__ __forceinline__ unsigned lane_mask_le()
{
    unsigned v;
    asm volatile("mov.u32 %0, %%lanemask_le;" : "=r"(v));
    return v;
}

/**
 * @brief 返回本 lane 右侧掩码（不含自身）。
 */
__device__ __forceinline__ unsigned lane_mask_gt()
{
    unsigned v;
    asm volatile("mov.u32 %0, %%lanemask_gt;" : "=r"(v));
    return v;
}

/**
 * @brief 返回本 lane 右侧掩码（含自身）。
 */
__device__ __forceinline__ unsigned lane_mask_ge()
{
    unsigned v;
    asm volatile("mov.u32 %0, %%lanemask_ge;" : "=r"(v));
    return v;
}

/**
 * @brief 返回掩码中最低位 lane id，掩码为 0 时返回 -1。
 */
__device__ __forceinline__ int first_lane(unsigned mask)
{
    return __ffs(mask) - 1;
}

/**
 * @brief 当前线程是否是掩码内的 leader（最低 lane）。
 */
__device__ __forceinline__ bool is_leader(unsigned mask = kFullMask)
{
    return lane_id() == first_lane(mask);
}

// =========================================================
// 同步与 lane 间数据交换
// =========================================================

/**
 * @brief Warp 级同步栅栏。
 *
 * 等待 mask 指定的 lane 全部到达此处后再继续执行。
 * 用于替代 __syncthreads 的轻量同步，保证 warp 内写入对其他参与线程可见。
 */
__device__ __forceinline__ void sync(unsigned mask = kFullMask)
{
    __syncwarp(mask);
}

/**
 * @brief Warp 内广播（任意 lane -> 全体）。
 *
 * 等价于 __shfl_sync，所有参与线程都获得 src_lane 的 value。
 */
template <typename T>
__device__ __forceinline__ T shfl(T value, int src_lane, unsigned mask = kFullMask)
{
    return __shfl_sync(mask, value, src_lane);
}

/**
 * @brief Warp 内向上搬运（lane i 读取 lane i-delta）。
 *
 * 常用于做前缀和或滑动窗口。
 */
template <typename T>
__device__ __forceinline__ T shfl_up(T value, unsigned delta, unsigned mask = kFullMask)
{
    return __shfl_up_sync(mask, value, delta);
}

/**
 * @brief Warp 内向下搬运（lane i 读取 lane i+delta）。
 *
 * 常用于归约或树形聚合。
 */
template <typename T>
__device__ __forceinline__ T shfl_down(T value, unsigned delta, unsigned mask = kFullMask)
{
    return __shfl_down_sync(mask, value, delta);
}

/**
 * @brief Warp 内 XOR 交换（lane i 读取 lane i ^ lane_mask）。
 *
 * 适合蝶形通信或位运算重排。
 */
template <typename T>
__device__ __forceinline__ T shfl_xor(T value, int lane_mask, unsigned mask = kFullMask)
{
    return __shfl_xor_sync(mask, value, lane_mask);
}

/**
 * @brief 在 warp 内广播某个 lane 的值给所有参与线程。
 *
 * @param value      每个线程本地值。
 * @param src_lane   作为广播源的 lane。
 * @param mask       参与通信的线程掩码。
 */
template <typename T>
__device__ __forceinline__ T broadcast(T value, int src_lane, unsigned mask = kFullMask)
{
    return __shfl_sync(mask, value, src_lane);
}

// =========================================================
// Warp 归约与扫描
// =========================================================

/**
 * @brief warp 归约求和（reduction sum）。
 *
 * 输入：每个线程一个局部值 value。
 * 输出：返回值仅在 lane 0 上是整个 warp 的和；其他 lane 的值可忽略。
 *
 * 使用场景：快速替代 shared memory + __syncthreads() 的 block 内末段归约。
 */
template <typename T>
__device__ __forceinline__ T reduce_sum(T value, unsigned mask = kFullMask)
{
    static_assert(std::is_arithmetic_v<T>, "reduce_sum requires arithmetic type");
    for (int offset = kWarpSize / 2; offset > 0; offset /= 2) {
        value += __shfl_down_sync(mask, value, offset);
    }
    return value;
}

/**
 * @brief warp 归约求最大值。
 *
 * 返回值仅在 lane 0 上有意义。
 */
template <typename T>
__device__ __forceinline__ T reduce_max(T value, unsigned mask = kFullMask)
{
    static_assert(std::is_arithmetic_v<T>, "reduce_max requires arithmetic type");
    for (int offset = kWarpSize / 2; offset > 0; offset /= 2) {
        const T other = __shfl_down_sync(mask, value, offset);
        value         = value > other ? value : other;
    }
    return value;
}

/**
 * @brief warp 归约求最小值。
 *
 * 返回值仅在 lane 0 上有意义。
 */
template <typename T>
__device__ __forceinline__ T reduce_min(T value, unsigned mask = kFullMask)
{
    static_assert(std::is_arithmetic_v<T>, "reduce_min requires arithmetic type");
    for (int offset = kWarpSize / 2; offset > 0; offset /= 2) {
        const T other = __shfl_down_sync(mask, value, offset);
        value         = value < other ? value : other;
    }
    return value;
}

/**
 * @brief Warp 归约求乘积（结果仅 lane0 有效）。
 */
template <typename T>
__device__ __forceinline__ T reduce_prod(T value, unsigned mask = kFullMask)
{
    for (int offset = kWarpSize / 2; offset > 0; offset >>= 1) {
        value *= __shfl_down_sync(mask, value, offset);
    }
    return value;
}

/**
 * @brief warp 全归约求和（all-reduce sum）。
 *
 * 与 reduce_sum 的区别：所有 lane 都会得到相同的总和。
 * 实现方式：先做归约，再把 lane 0 的结果广播给所有线程。
 */
template <typename T>
__device__ __forceinline__ T all_reduce_sum(T value, unsigned mask = kFullMask)
{
    value = reduce_sum(value, mask);
    return broadcast(value, 0, mask);
}

/**
 * @brief Warp 全归约最小值，所有 lane 得到相同结果。
 */
template <typename T>
__device__ __forceinline__ T all_reduce_min(T value, unsigned mask = kFullMask)
{
    value = reduce_min(value, mask);
    return broadcast(value, 0, mask);
}

/**
 * @brief Warp 全归约最大值，所有 lane 得到相同结果。
 */
template <typename T>
__device__ __forceinline__ T all_reduce_max(T value, unsigned mask = kFullMask)
{
    value = reduce_max(value, mask);
    return broadcast(value, 0, mask);
}

/**
 * @brief Warp 全归约乘积，所有 lane 得到相同结果。
 */
template <typename T>
__device__ __forceinline__ T all_reduce_prod(T value, unsigned mask = kFullMask)
{
    value = reduce_prod(value, mask);
    return broadcast(value, 0, mask);
}

// 将 lane0 的值广播到当前 mask 覆盖的所有 lane。
template <typename T>
__device__ __forceinline__ T broadcast_first(T value, unsigned mask = kFullMask)
{
    return __shfl_sync(mask, value, 0);
}

/**
 * @brief warp 内前缀和（inclusive scan）。
 *
 * inclusive 的定义：
 * lane i 的结果 = lane[0..i] 的和（仅统计 mask 中活跃线程）。
 *
 * 注意：该实现假设活跃线程在 lane 维度上“前缀连续”最稳妥。
 */
template <typename T>
__device__ __forceinline__ T inclusive_prefix_sum(T value, unsigned mask = kFullMask)
{
    const int lane = lane_id();
    for (int offset = 1; offset < kWarpSize; offset <<= 1) {
        const T n = __shfl_up_sync(mask, value, offset);
        if (lane >= offset) {
            value += n;
        }
    }
    return value;
}

/**
 * @brief warp 内前缀和（exclusive scan）。
 *
 * exclusive 的定义：
 * lane i 的结果 = lane[0..i-1] 的和。
 */
template <typename T>
__device__ __forceinline__ T exclusive_prefix_sum(T value, unsigned mask = kFullMask)
{
    const T inclusive = inclusive_prefix_sum(value, mask);
    return inclusive - value;
}

// =========================================================
// 四、投票、匹配与压缩索引
// =========================================================

/**
 * @brief warp 级“是否存在”投票。
 *
 * 返回 true 表示：在 mask 指定的线程中，至少有一个线程 predicate 为 true。
 */
__device__ __forceinline__ bool any(bool predicate, unsigned mask = kFullMask)
{
    return __any_sync(mask, predicate);
}

/**
 * @brief warp 级“是否全部满足”投票。
 *
 * 返回 true 表示：在 mask 指定的线程中，所有线程 predicate 都为 true。
 */
__device__ __forceinline__ bool all(bool predicate, unsigned mask = kFullMask)
{
    return __all_sync(mask, predicate);
}

/**
 * @brief warp 级 ballot 投票。
 *
 * 返回一个 32-bit 掩码：
 * - 第 i 位为 1 表示 lane i 的 predicate 为 true；
 * - 第 i 位为 0 表示 predicate 为 false 或 lane 不活跃。
 */
__device__ __forceinline__ unsigned ballot(bool predicate, unsigned mask = kFullMask)
{
    return __ballot_sync(mask, predicate);
}

/**
 * @brief Warp 内一致性判断。
 *
 * 返回 true 表示 mask 指定的线程中 predicate 全部一致（全真或全假）。
 */
__device__ __forceinline__ bool uniform(bool predicate, unsigned mask = kFullMask)
{
    return __uni_sync(mask, predicate);
}

/**
 * @brief 返回与当前线程 value 相等的 lane 掩码。
 *
 * 用于聚合原子、分组统计等场景。
 */
__device__ __forceinline__ unsigned match_any(unsigned value, unsigned mask = kFullMask)
{
    return __match_any_sync(mask, value);
}

/**
 * @brief 返回与当前线程 value 相等的 lane 掩码（int）。
 */
__device__ __forceinline__ unsigned match_any(int value, unsigned mask = kFullMask)
{
    return __match_any_sync(mask, value);
}

/**
 * @brief 返回与当前线程 value 相等的 lane 掩码（float）。
 */
__device__ __forceinline__ unsigned match_any(float value, unsigned mask = kFullMask)
{
    return __match_any_sync(mask, value);
}

/**
 * @brief 返回与当前线程 value 相等的 lane 掩码（double）。
 */
__device__ __forceinline__ unsigned match_any(double value, unsigned mask = kFullMask)
{
    return __match_any_sync(mask, value);
}

/**
 * @brief 返回相同值的 lane 掩码，并告知是否“全员一致”。
 *
 * pred 为 true 表示 mask 内所有线程 value 相等。
 */
__device__ __forceinline__ unsigned match_all(unsigned value, bool& pred, unsigned mask = kFullMask)
{
    int p           = 0;
    unsigned result = __match_all_sync(mask, value, &p);
    pred            = p != 0;
    return result;
}

/**
 * @brief match_all 的 int 版本。
 */
__device__ __forceinline__ unsigned match_all(int value, bool& pred, unsigned mask = kFullMask)
{
    int p           = 0;
    unsigned result = __match_all_sync(mask, value, &p);
    pred            = p != 0;
    return result;
}

/**
 * @brief match_all 的 float 版本。
 */
__device__ __forceinline__ unsigned match_all(float value, bool& pred, unsigned mask = kFullMask)
{
    int p           = 0;
    unsigned result = __match_all_sync(mask, value, &p);
    pred            = p != 0;
    return result;
}

/**
 * @brief match_all 的 double 版本。
 */
__device__ __forceinline__ unsigned match_all(double value, bool& pred, unsigned mask = kFullMask)
{
    int p           = 0;
    unsigned result = __match_all_sync(mask, value, &p);
    pred            = p != 0;
    return result;
}

/**
 * @brief 计算当前线程在 warp 内的“压缩写入下标”（stream compaction 常用）。
 *
 * 给定 predicate=true 的线程需要紧凑写出，返回当前线程在本 warp 内应写入的位置。
 * 例如 predicate 在 lane [0,2,5] 为 true，则它们的索引分别是 [0,1,2]。
 */
__device__ __forceinline__ int compact_index(bool predicate, unsigned mask = kFullMask)
{
    const unsigned vote = __ballot_sync(mask, predicate);
    const int lane      = lane_id();
    const unsigned left = vote & ((1u << lane) - 1u);
    return __popc(left);
}

/**
 * @brief 统计 warp 内 predicate=true 的线程数。
 */
__device__ __forceinline__ int count_if(bool predicate, unsigned mask = kFullMask)
{
    return __popc(__ballot_sync(mask, predicate));
}

} // namespace bee::cuda
