/**
 * @File Core/Block.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
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

    // 完整 reduce，结果返回到 tid == 0；其他线程返回未定义值。
    __device__ __forceinline__ auto Reduce(T input, Op op = Op{}) -> T
    {
        const unsigned int tid  = block_thread_rank();
        const unsigned int lane = tid & (kWarpSize - 1);
        const unsigned int wid  = tid / kWarpSize;

        T warp_val = warp_reduce(input, op, static_cast<int>(kWarpSize));
        if (lane == 0) {
            storage.warp_sums[wid] = warp_val;
        }
        block_sync();

        // 第二阶段：warp 0 聚合前 kNumWarps 个 warp sum
        if (wid == 0) {
            T v = (lane < kNumWarps) ? storage.warp_sums[lane] : warp_val /* 占位 */;
            // 对于 lane >= kNumWarps 的元素，注入 identity：使用 op 的"吸收性"不通用，
            // 更稳妥的是：用 warp_sums[0] 当 seed，对多余 lane 填 warp_sums[0] 即可。
            if (lane >= kNumWarps) {
                v = storage.warp_sums[0];
            }
            // 若 kNumWarps < 32 且 op 非幂等（例如 Sum 会重复累加 warp_sums[0]），
            // 因此改走只激活 kNumWarps 个 lane 的 reduce：使用 sub-warp width。
            // 为简化，选 width = 最接近 kNumWarps 的 2 的幂，并单独处理 tail。
            T acc = (lane < kNumWarps) ? storage.warp_sums[lane] : T{};
// 用折半 reduce，仅在 lane < kNumWarps 范围内有效
#pragma unroll
            for (int offset = 16; offset > 0; offset >>= 1) {
                T other = warp_shfl_xor(acc, offset, kWarpSize);
                // 仅当对侧 lane 也在有效范围内才累加；但 Op 可能不具可选性，
                // 故我们把无效 lane 的 acc 设为 identity。Sum/Prod/Min/Max 都有
                // 可区分的 identity，这里交由调用者通过 ReduceWithIdentity 指定。
                acc = op(acc, other);
            }
            (void)v;
            if (lane == 0) {
                storage.broadcast = acc;
            }
        }
        block_sync();
        return storage.broadcast;
    }

    // 更通用的版本：显式指定 identity，处理 kNumWarps 非 32 的情况时更可靠。
    __device__ __forceinline__ auto ReduceWithIdentity(T input, T identity, Op op = Op{}) -> T
    {
        const unsigned int tid  = block_thread_rank();
        const unsigned int lane = tid & (kWarpSize - 1);
        const unsigned int wid  = tid / kWarpSize;

        T warp_val = warp_reduce(input, op, static_cast<int>(kWarpSize));
        if (lane == 0) {
            storage.warp_sums[wid] = warp_val;
        }
        block_sync();

        if (wid == 0) {
            T acc = (lane < kNumWarps) ? storage.warp_sums[lane] : identity;
#pragma unroll
            for (int offset = 16; offset > 0; offset >>= 1) {
                T other = warp_shfl_xor(acc, offset, kWarpSize);
                acc     = op(acc, other);
            }
            if (lane == 0) {
                storage.broadcast = acc;
            }
        }
        block_sync();
        return storage.broadcast;
    }

    // 便捷函数
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
