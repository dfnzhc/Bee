/**
 * @File Core/Block.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief CUDA block 级并行原语。
 *
 * Block 级并行原语：基于 Warp.cuh 的两阶段（warp → shared → warp）实现。
 * CUB 风格 TempStorage：调用方负责声明 __shared__，便于 kernel 内多次复用。
 *
 * 要求 blockDim.x * blockDim.y * blockDim.z <= 1024（CUDA 硬上限）。
 * 约定：线程线性编号使用 tid = threadIdx.x + threadIdx.y * blockDim.x +
 *                       threadIdx.z * blockDim.x * blockDim.y。
 */

#pragma once

#include <cuda_runtime.h>

#include <cstdint>

#include "CUDA/Core/Warp.cuh"

namespace bee::cuda
{

__device__ __forceinline__ auto block_sync() -> void
{
    __syncthreads();
}

__device__ __forceinline__ auto block_thread_count() -> unsigned int
{
    return blockDim.x * blockDim.y * blockDim.z;
}

__device__ __forceinline__ auto block_thread_rank() -> unsigned int
{
    return threadIdx.x + threadIdx.y * blockDim.x + threadIdx.z * blockDim.x * blockDim.y;
}

// ────────────────────────────────────────────────────────────────────────────
// BlockReduce<T, BlockSize, Op>
//   - BlockSize 必须 ≤ 1024；内部按 warp 划分（向上取整到 32 的倍数 warp 数）。
//   - 两阶段：
//       1) 每个 warp 做 warp_reduce；
//       2) 各 warp 的 lane 0 把部分和写入 shared，warp 0 再做一次 reduce 并广播。
//   - 结果仅在 linear tid == 0 有效；如需全 block 广播，在 Reduce 后读取
//     smem.broadcast 再 __syncthreads()。
// ────────────────────────────────────────────────────────────────────────────

template <typename T, int BlockSize, typename Op = WarpOpSum>
struct BlockReduce
{
    static_assert(BlockSize > 0 && BlockSize <= 1024, "BlockSize 需在 (0,1024]");
    static constexpr int kNumWarps = (BlockSize + kWarpSize - 1) / kWarpSize;

    // 把 kNumWarps 向上取整到 2 的幂，用于第二阶段 shuffle reduce 的 width。
    // 这样可以在 lane >= kNumWarps 注入 identity 后安全 reduce，避免对超出部分
    // 做多余 shuffle 轮数。
    static constexpr int kAggrWidthP2 = []() constexpr {
        int w = 1;
        while (w < kNumWarps)
            w <<= 1;
        return w;
    }();

    struct TempStorage
    {
        T warp_sums[kNumWarps];
        T broadcast;
    };

    TempStorage& storage;

    __device__ __forceinline__ explicit BlockReduce(TempStorage& s)
        : storage(s)
    {
    }

    // 通用 reduce，显式指定 identity（Sum=0，Prod=1，Min=MAX，Max=MIN 等）。
    // 结果广播到 block 内所有线程（从 storage.broadcast 读出）。
    __device__ __forceinline__ auto ReduceWithIdentity(T input, T identity, Op op = Op{}) -> T
    {
        const unsigned int tid  = block_thread_rank();
        const unsigned int lane = tid & (kWarpSize - 1);
        const unsigned int wid  = tid / kWarpSize;

        // 第一阶段：每个 warp 内 reduce，lane 0 写 smem
        T warp_val = warp_reduce(input, op, static_cast<int>(kWarpSize));
        if (lane == 0) {
            storage.warp_sums[wid] = warp_val;
        }
        block_sync();

        // 第二阶段：warp 0 对 kNumWarps 条 partial 做 reduce
        if (wid == 0) {
            // lane >= kNumWarps 的线程注入 identity，保证 butterfly reduce 正确
            T acc = (static_cast<int>(lane) < kNumWarps) ? storage.warp_sums[lane] : identity;

            // 只在 next_pow2(kNumWarps) 宽度内折半 —— 对 kNumWarps=8 从 5 轮减到 3 轮
            if constexpr (kAggrWidthP2 > 1) {
                BEE_UNROLL
                for (int offset = kAggrWidthP2 / 2; offset > 0; offset >>= 1) {
                    T other = warp_shfl_xor(acc, offset, kWarpSize);
                    acc     = op(acc, other);
                }
            }

            if (lane == 0) {
                storage.broadcast = acc;
            }
        }
        block_sync();
        return storage.broadcast;
    }

    // 便捷函数：Sum 固定 identity = T{0}
    __device__ __forceinline__ auto Sum(T input) -> T
    {
        return ReduceWithIdentity(input, T{0}, WarpOpSum{});
    }

    __device__ __forceinline__ auto Min(T input, T identity) -> T
    {
        return ReduceWithIdentity(input, identity, WarpOpMin{});
    }

    __device__ __forceinline__ auto Max(T input, T identity) -> T
    {
        return ReduceWithIdentity(input, identity, WarpOpMax{});
    }

    // 复用 TempStorage::broadcast 做一次 block 广播（节省 smem，避免再单开
    // BlockBroadcast::TempStorage）。需在 Reduce 后调用。
    __device__ __forceinline__ auto Broadcast(T input, unsigned int src_tid) -> T
    {
        if (block_thread_rank() == src_tid) {
            storage.broadcast = input;
        }
        block_sync();
        return storage.broadcast;
    }
};

// ────────────────────────────────────────────────────────────────────────────
// BlockScan<T, BlockSize>（仅实现 Sum scan，通用 op 可自行扩展）
//   - inclusive / exclusive
//   - 采用 warp-scan → warp-aggregate → add back 三段式
// ────────────────────────────────────────────────────────────────────────────

template <typename T, int BlockSize>
struct BlockScan
{
    static_assert(BlockSize > 0 && BlockSize <= 1024, "BlockSize 需在 (0,1024]");
    static constexpr int kNumWarps = (BlockSize + kWarpSize - 1) / kWarpSize;

    struct TempStorage
    {
        T warp_totals[kNumWarps];
    };

    TempStorage& storage;

    __device__ __forceinline__ explicit BlockScan(TempStorage& s)
        : storage(s)
    {
    }

    // Inclusive sum scan；每个线程返回自己位置的前缀（含自身）。
    __device__ __forceinline__ auto InclusiveSum(T input) -> T
    {
        const unsigned int tid  = block_thread_rank();
        const unsigned int lane = tid & (kWarpSize - 1);
        const unsigned int wid  = tid / kWarpSize;

        T warp_inc = warp_scan_inclusive_sum(input);
        if (lane == kWarpSize - 1) {
            storage.warp_totals[wid] = warp_inc;
        }
        block_sync();

        // 用 warp 0 对 warp_totals 做 exclusive scan，得到每个 warp 的前缀偏移
        if (wid == 0) {
            T v  = (lane < kNumWarps) ? storage.warp_totals[lane] : T{0};
            T ex = warp_scan_exclusive_sum(v);
            if (lane < kNumWarps) {
                storage.warp_totals[lane] = ex;
            }
        }
        block_sync();

        return warp_inc + storage.warp_totals[wid];
    }

    // Exclusive sum scan；tid 0 返回 0。
    __device__ __forceinline__ auto ExclusiveSum(T input) -> T
    {
        T inc = InclusiveSum(input);
        return inc - input;
    }
};

// ────────────────────────────────────────────────────────────────────────────
// block_broadcast<T>(value, src_tid)
//   任意线程把 value 广播给 block 内所有线程。需要 TempStorage（单个 T）。
// ────────────────────────────────────────────────────────────────────────────

template <typename T>
struct BlockBroadcast
{
    struct TempStorage
    {
        T value;
    };

    TempStorage& storage;

    __device__ __forceinline__ explicit BlockBroadcast(TempStorage& s)
        : storage(s)
    {
    }

    __device__ __forceinline__ auto Broadcast(T input, unsigned int src_tid) -> T
    {
        if (block_thread_rank() == src_tid) {
            storage.value = input;
        }
        block_sync();
        return storage.value;
    }
};

} // namespace bee::cuda
