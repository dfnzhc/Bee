/**
 * @File Core/Warp.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief CUDA warp 级并行原语。
 *
 * Warp 级（32 lane）并行原语集合。
 *
 * 依赖 __shfl_sync / __ballot_sync / __any_sync / __all_sync 等 sm_80+ 普遍可用的
 * intrinsics。本组件仅服务于 sm_120（Blackwell），不做更低架构的兼容。
 *
 * 所有函数均为 __device__ __forceinline__，可在任意 warp-divergent 上下文使用，
 * 但调用前需保证 mask 指定的 lane 全部到达（否则为未定义行为）。默认 mask = 0xFFFFFFFFu。
 */

#pragma once

#include "CUDA/Core/Check.cuh"
#include <cuda_runtime.h>

#include <cstdint>
#include <type_traits>

namespace bee::cuda
{

inline constexpr unsigned int kWarpSize   = 32u;
inline constexpr unsigned int kFullMask32 = 0xFFFFFFFFu;

// ────────────────────────────────────────────────────────────────────────────
// 底层 shuffle：32-bit 通用实现 + 对任意可平凡复制的 T 做分段 shuffle。
// T 被切成若干 32-bit chunk 独立 shuffle，再按位拼回，适用于 double/long long/
// 自定义 POD 结构。
// ────────────────────────────────────────────────────────────────────────────

namespace detail
{

    template <typename T, typename Op>
    __device__ __forceinline__ auto shuffle_poly(T value, Op op) -> T
    {
        static_assert(std::is_trivially_copyable_v<T>, "warp shuffle 仅支持 trivially copyable 类型");
        constexpr int num_words = (sizeof(T) + sizeof(int) - 1) / sizeof(int);

        // 按 32-bit 切片，单独做 shuffle
        union
        {
            T   v;
            int words[num_words];
        } src{value}, dst{};

        BEE_UNROLL
        for (int i = 0; i < num_words; ++i) {
            dst.words[i] = op(src.words[i]);
        }
        return dst.v;
    }

} // namespace detail

// ────────────────────────────────────────────────────────────────────────────
// Shuffle 原语
// ────────────────────────────────────────────────────────────────────────────

// 从 src_lane 读取 value
template <typename T>
__device__ __forceinline__ auto warp_shfl(T value, int src_lane, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    return detail::shuffle_poly(value, [&](int w) { return __shfl_sync(mask, w, src_lane, width); });
}

// 从 laneid XOR xor_mask 读取 value（butterfly 交换）
template <typename T>
__device__ __forceinline__ auto warp_shfl_xor(T value, int xor_mask, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    return detail::shuffle_poly(value, [&](int w) { return __shfl_xor_sync(mask, w, xor_mask, width); });
}

// 从 laneid - delta 读取 value（向上）
template <typename T>
__device__ __forceinline__ auto warp_shfl_up(T value, unsigned int delta, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    return detail::shuffle_poly(value, [&](int w) { return __shfl_up_sync(mask, w, delta, width); });
}

// 从 laneid + delta 读取 value（向下）
template <typename T>
__device__ __forceinline__ auto warp_shfl_down(T value, unsigned int delta, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    return detail::shuffle_poly(value, [&](int w) { return __shfl_down_sync(mask, w, delta, width); });
}

// 从 src_lane 读并广播到 warp 内所有 lane
template <typename T>
__device__ __forceinline__ auto warp_broadcast(T value, int src_lane, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    return warp_shfl(value, src_lane, width, mask);
}

// ────────────────────────────────────────────────────────────────────────────
// 谓词原语
// ────────────────────────────────────────────────────────────────────────────

__device__ __forceinline__ auto warp_any(bool pred, unsigned int mask = kFullMask32) -> bool
{
    return __any_sync(mask, pred) != 0;
}

__device__ __forceinline__ auto warp_all(bool pred, unsigned int mask = kFullMask32) -> bool
{
    return __all_sync(mask, pred) != 0;
}

__device__ __forceinline__ auto warp_ballot(bool pred, unsigned int mask = kFullMask32) -> unsigned int
{
    return __ballot_sync(mask, pred);
}

// ────────────────────────────────────────────────────────────────────────────
// Reduce 二元操作符（内置）
// ────────────────────────────────────────────────────────────────────────────

struct WarpOpSum
{
    template <typename T>
    __device__ __forceinline__ auto operator()(T a, T b) const -> T
    {
        return a + b;
    }
};

struct WarpOpProd
{
    template <typename T>
    __device__ __forceinline__ auto operator()(T a, T b) const -> T
    {
        return a * b;
    }
};

struct WarpOpMin
{
    template <typename T>
    __device__ __forceinline__ auto operator()(T a, T b) const -> T
    {
        return a < b ? a : b;
    }
};

struct WarpOpMax
{
    template <typename T>
    __device__ __forceinline__ auto operator()(T a, T b) const -> T
    {
        return a > b ? a : b;
    }
};

// ────────────────────────────────────────────────────────────────────────────
// Reduce：butterfly 折半，结果广播到整个 warp（或子 warp width）
// ────────────────────────────────────────────────────────────────────────────

template <typename T, typename Op>
__device__ __forceinline__ auto warp_reduce(T value, Op op, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    BEE_UNROLL
    for (int offset = width / 2; offset > 0; offset >>= 1) {
        T other = warp_shfl_xor(value, offset, width, mask);
        value   = op(value, other);
    }
    return value;
}

// 说明：对 int32 / uint32 类型，在整个 warp（width = 32 且 mask = kFullMask32）上的
// reduce 可以直接使用 sm_80+ 的 __reduce_*_sync 内建（单指令完成 warp 级归约），
// 比 5 轮 shuffle 折半快 ~3x。其他宽度、带 mask、或其他类型仍走 butterfly。

template <typename T>
__device__ __forceinline__ auto warp_reduce_sum(T value, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    if constexpr (std::is_same_v<T, int> || std::is_same_v<T, unsigned int>) {
        if (width == static_cast<int>(kWarpSize) && mask == kFullMask32) {
            return static_cast<T>(__reduce_add_sync(mask, static_cast<unsigned int>(value)));
        }
    }
    return warp_reduce(value, WarpOpSum{}, width, mask);
}

template <typename T>
__device__ __forceinline__ auto warp_reduce_prod(T value, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    return warp_reduce(value, WarpOpProd{}, width, mask);
}

template <typename T>
__device__ __forceinline__ auto warp_reduce_min(T value, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    if constexpr (std::is_same_v<T, int>) {
        if (width == static_cast<int>(kWarpSize) && mask == kFullMask32) {
            return static_cast<T>(__reduce_min_sync(mask, static_cast<int>(value)));
        }
    } else if constexpr (std::is_same_v<T, unsigned int>) {
        if (width == static_cast<int>(kWarpSize) && mask == kFullMask32) {
            return static_cast<T>(__reduce_min_sync(mask, static_cast<unsigned int>(value)));
        }
    }
    return warp_reduce(value, WarpOpMin{}, width, mask);
}

template <typename T>
__device__ __forceinline__ auto warp_reduce_max(T value, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    if constexpr (std::is_same_v<T, int>) {
        if (width == static_cast<int>(kWarpSize) && mask == kFullMask32) {
            return static_cast<T>(__reduce_max_sync(mask, static_cast<int>(value)));
        }
    } else if constexpr (std::is_same_v<T, unsigned int>) {
        if (width == static_cast<int>(kWarpSize) && mask == kFullMask32) {
            return static_cast<T>(__reduce_max_sync(mask, static_cast<unsigned int>(value)));
        }
    }
    return warp_reduce(value, WarpOpMax{}, width, mask);
}

// ────────────────────────────────────────────────────────────────────────────
// Scan（前缀和）：Hillis-Steele 风格，O(log2 width) 次 shuffle
// inclusive：结果 lane i = sum_{k<=i}
// exclusive：结果 lane 0 = 0，lane i = sum_{k<i}
// ────────────────────────────────────────────────────────────────────────────

template <typename T, typename Op>
__device__ __forceinline__ auto warp_scan_inclusive(T value, Op op, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    const unsigned int lane = threadIdx.x & (kWarpSize - 1);
    BEE_UNROLL
    for (int offset = 1; offset < width; offset <<= 1) {
        T up = warp_shfl_up(value, static_cast<unsigned int>(offset), width, mask);
        if (static_cast<int>(lane) >= offset) {
            value = op(value, up);
        }
    }
    return value;
}

template <typename T>
__device__ __forceinline__ auto warp_scan_inclusive_sum(T value, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    return warp_scan_inclusive(value, WarpOpSum{}, width, mask);
}

// exclusive_sum：inclusive 向右挪一位，lane 0 填 T{}
template <typename T>
__device__ __forceinline__ auto warp_scan_exclusive_sum(T value, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    T inc  = warp_scan_inclusive_sum(value, width, mask);
    T prev = warp_shfl_up(inc, 1u, width, mask);

    const unsigned int lane = threadIdx.x & (kWarpSize - 1);
    return lane == 0 ? T{} : prev;
}

// 通用 exclusive scan（带初始 identity 值）
template <typename T, typename Op>
__device__ __forceinline__ auto warp_scan_exclusive(T value, Op op, T identity, int width = kWarpSize, unsigned int mask = kFullMask32) -> T
{
    T inc  = warp_scan_inclusive(value, op, width, mask);
    T prev = warp_shfl_up(inc, 1u, width, mask);

    const unsigned int lane = threadIdx.x & (kWarpSize - 1);
    return lane == 0 ? identity : prev;
}

} // namespace bee::cuda
