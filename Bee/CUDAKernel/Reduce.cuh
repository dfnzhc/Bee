/**
 * @File Reduce.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Launch.cuh"

#include <cuda_runtime.h>

#include <concepts>
#include <type_traits>

namespace bee::cuda
{

// =========================================================
// 基础 block 归约 kernel
// =========================================================

/**
 * @brief 单阶段分块归约：把输入数组归约到每个 block 一个结果。
 *
 * 输入：
 * - in:     当前阶段输入数组
 * - n:      输入长度
 * - identity: 该归约运算的幺元（如 sum 的 0，max 的最小值）
 * 输出：
 * - partial_out[blockIdx.x] = 本 block 的局部归约结果
 */
template <typename T, typename Op, int BlockSize>
__global__ void reduce_blocks_kernel(const T* in, T* partial_out, int n, T identity, Op op)
{
    __shared__ T sdata[BlockSize];

    const int tid        = static_cast<int>(threadIdx.x);
    const int global_idx = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int stride     = static_cast<int>(blockDim.x * gridDim.x);

    // 每个线程先做 grid-stride 局部累积，再写入共享内存。
    T local = identity;
    for (int i = global_idx; i < n; i += stride) {
        local = static_cast<T>(op(local, in[i]));
    }
    sdata[tid] = local;
    __syncthreads();

    // 共享内存树形归约到 block 内单值。
    for (int offset = BlockSize / 2; offset > 0; offset >>= 1) {
        if (tid < offset) {
            sdata[tid] = static_cast<T>(op(sdata[tid], sdata[tid + offset]));
        }
        __syncthreads();
    }

    if (tid == 0) {
        partial_out[blockIdx.x] = sdata[0];
    }
}

/**
 * @brief 空输入分支：直接写 identity 作为归约结果。
 */
template <typename T>
__global__ void write_identity_kernel(T* out, T identity)
{
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        out[0] = identity;
    }
}

// =========================================================
// 三、Launch 封装（多阶段归约）
// =========================================================

/**
 * @brief 设备端归约入口：把 in[0..n) 归约到 out[0]。
 *
 * 语义：
 * - n <= 0 时，out[0] = identity。
 * - n > 0 时，out[0] = reduce(in, op, identity)。
 *
 * 实现：
 * - 使用两块临时缓冲做 ping-pong，多阶段把长度逐步缩小到 1。
 */
template <typename T, typename Op>
cudaError_t launch_reduce(const T* in, T* out, int n, T identity, Op op, cudaStream_t stream = nullptr)
{
    constexpr int kBlock = 256;

    if (n <= 0) {
        write_identity_kernel<<<1, 1, 0, stream>>>(out, identity);
        return cudaGetLastError();
    }

    // 第一阶段最多会产生 ceil(n / kBlock) 个 partial。
    const int max_partial = ceil_div(n, kBlock);

    T* buf_a = nullptr;
    T* buf_b = nullptr;

    cudaError_t err = cudaMalloc(&buf_a, static_cast<std::size_t>(max_partial) * sizeof(T));
    if (err != cudaSuccess) {
        return err;
    }

    err = cudaMalloc(&buf_b, static_cast<std::size_t>(max_partial) * sizeof(T));
    if (err != cudaSuccess) {
        cudaFree(buf_a);
        return err;
    }

    const T* cur_in = in;
    int cur_n       = n;
    bool write_to_a = true;

    while (cur_n > 1) {
        const int grid = ceil_div(cur_n, kBlock);
        T* cur_out     = write_to_a ? buf_a : buf_b;

        reduce_blocks_kernel<T, Op, kBlock><<<grid, kBlock, 0, stream>>>(cur_in, cur_out, cur_n, identity, op);
        err = cudaGetLastError();
        if (err != cudaSuccess) {
            cudaFree(buf_a);
            cudaFree(buf_b);
            return err;
        }

        cur_in     = cur_out;
        cur_n      = grid;
        write_to_a = !write_to_a;
    }

    err = cudaMemcpyAsync(out, cur_in, sizeof(T), cudaMemcpyDeviceToDevice, stream);

    cudaFree(buf_a);
    cudaFree(buf_b);
    return err;
}

/**
 * @brief 便捷接口：归约后直接回传到 host 标量。
 */
template <typename T, typename Op>
cudaError_t reduce_to_host(const T* in, int n, T identity, Op op, T* host_out, cudaStream_t stream = nullptr)
{
    T* d_out        = nullptr;
    cudaError_t err = cudaMalloc(&d_out, sizeof(T));
    if (err != cudaSuccess) {
        return err;
    }

    err = launch_reduce(in, d_out, n, identity, op, stream);
    if (err == cudaSuccess) {
        err = cudaMemcpyAsync(host_out, d_out, sizeof(T), cudaMemcpyDeviceToHost, stream);
    }
    if (err == cudaSuccess) {
        err = cudaStreamSynchronize(stream);
    }

    cudaFree(d_out);
    return err;
}

} // namespace bee::cuda
