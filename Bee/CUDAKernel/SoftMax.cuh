/**
 * @File SoftMax.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Block.cuh"
#include "Launch.cuh"
#include "Vectorize.cuh"

#include <cuda_fp16.h>
#include <cuda_runtime.h>

#include <cstddef>
#include <type_traits>

namespace bee::cuda
{

// =========================================================
// Softmax 数学定义与实现说明
// ---------------------------------------------------------
// 对于一行向量 x（长度为 C），softmax 定义为：
//   y_i = exp(x_i) / sum_j exp(x_j)
//
// 为避免数值溢出，采用稳定写法：
//   m = max_j x_j
//   y_i = exp(x_i - m) / sum_j exp(x_j - m)
//
// 本实现采用“每个 block 处理一行”的并行策略，支持：
// 1) float 标量路径
// 2) float4 向量化路径（对齐且列数可被 4 整除时）
// 3) half 输入/输出路径（累加在 float 中）
//
// 进阶功能选择：
// - 通过构建宏 GPU_HAS_CUTLASS / GPU_HAS_WGMMA 预留了更高阶实现入口；
// - 当前 softmax 的主性能路径为向量化访存 + block 归约。
// =========================================================

template <typename T, int BlockSize>
__global__ void softmax_rowwise_scalar_kernel(const T* x, T* y, int rows, int cols)
{
    using AccT = AccumType_t<T>;

    const int row = static_cast<int>(blockIdx.x);
    if (row >= rows) {
        return;
    }

    const int tid  = static_cast<int>(threadIdx.x);
    const int base = row * cols;

    // 第一阶段：求该行最大值。
    AccT local_max = static_cast<AccT>(-1e20f);
    for (int c = tid; c < cols; c += BlockSize) {
        const AccT v = to_accum(x[base + c]);
        local_max    = local_max > v ? local_max : v;
    }
    const AccT row_max = reduce_max<AccT, BlockSize>(local_max);

    // 第二阶段：求分母 sum(exp(x - max))。
    AccT local_sum = static_cast<AccT>(0);
    for (int c = tid; c < cols; c += BlockSize) {
        const AccT v  = to_accum(x[base + c]);
        local_sum    += __expf(v - row_max);
    }
    const AccT row_sum = reduce_sum<AccT, BlockSize>(local_sum);

    // 第三阶段：归一化写回。
    for (int c = tid; c < cols; c += BlockSize) {
        const AccT v = to_accum(x[base + c]);
        const AccT o = __expf(v - row_max) / row_sum;
        y[base + c]  = from_accum<T>(o);
    }
}

template <int BlockSize>
__global__ void softmax_rowwise_f32_vec4_kernel(const float* x, float* y, int rows, int cols)
{
    using Pack4 = Pack<float, 4>;

    const int row = static_cast<int>(blockIdx.x);
    if (row >= rows) {
        return;
    }

    const int tid       = static_cast<int>(threadIdx.x);
    const int base      = row * cols;
    const int pack_cols = cols / 4;

    float local_max = -1e20f;
    for (int p = tid; p < pack_cols; p += BlockSize) {
        const Pack4 v = load_pack<Pack4>(x + base, p);
        local_max     = fmaxf(local_max, v.data[0]);
        local_max     = fmaxf(local_max, v.data[1]);
        local_max     = fmaxf(local_max, v.data[2]);
        local_max     = fmaxf(local_max, v.data[3]);
    }
    const float row_max = reduce_max<float, BlockSize>(local_max);

    float local_sum = 0.0f;
    for (int p = tid; p < pack_cols; p += BlockSize) {
        const Pack4 v  = load_pack<Pack4>(x + base, p);
        local_sum     += __expf(v.data[0] - row_max);
        local_sum     += __expf(v.data[1] - row_max);
        local_sum     += __expf(v.data[2] - row_max);
        local_sum     += __expf(v.data[3] - row_max);
    }
    const float row_sum = reduce_sum<float, BlockSize>(local_sum);

    for (int p = tid; p < pack_cols; p += BlockSize) {
        const Pack4 v = load_pack<Pack4>(x + base, p);
        Pack4 o{};
        o.data[0] = __expf(v.data[0] - row_max) / row_sum;
        o.data[1] = __expf(v.data[1] - row_max) / row_sum;
        o.data[2] = __expf(v.data[2] - row_max) / row_sum;
        o.data[3] = __expf(v.data[3] - row_max) / row_sum;
        store_pack<Pack4>(y + base, p, o);
    }
}

/**
 * @brief 行级 softmax 启动函数。
 *
 * 参数：
 * - x/y: 形状为 [rows, cols] 的连续内存。
 * - rows/cols: 行列尺寸。
 * - impl: 实现选择。Auto 会自动选择向量化路径（若可用）。
 */
template <typename T>
inline cudaError_t launch_softmax(const T* x, T* y, int rows, int cols, cudaStream_t stream = nullptr)
{
    if (rows <= 0 || cols <= 0) {
        return cudaSuccess;
    }

    constexpr int kBlock = 256;
    const dim3 grid(static_cast<unsigned>(rows), 1, 1);
    const dim3 block(static_cast<unsigned>(kBlock), 1, 1);

    bool use_vec = false;
    {
        using Pack4             = Pack<float, 4>;
        const bool can_vec      = std::is_same_v<T, float> && (cols % 4) == 0 && can_vectorize_ptr<Pack4>(x) && can_vectorize_ptr<Pack4>(y);
        const bool can_advanced = kBuildHasAdvancedPath && check_sm_level(8); // sm >= 8
        // 具备高阶能力时，Auto 在更小列宽也启用向量化；否则只在中等以上列宽启用。
        use_vec = can_vec && (can_advanced || cols >= 64);
    }

    if (use_vec) {
        if constexpr (std::is_same_v<T, float>) {
            softmax_rowwise_f32_vec4_kernel<kBlock><<<grid, block, 0, stream>>>(x, y, rows, cols);
            return cudaGetLastError();
        }
    }

    softmax_rowwise_scalar_kernel<T, kBlock><<<grid, block, 0, stream>>>(x, y, rows, cols);
    return cudaGetLastError();
}

} // namespace bee::cuda
