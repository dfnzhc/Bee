/**
 * @File LayerNorm.cuh
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
// LayerNorm 数学定义与实现说明
// ---------------------------------------------------------
// 对每一行向量 x（长度 C）做归一化：
//   mean = (1/C) * sum_j x_j
//   var  = (1/C) * sum_j (x_j - mean)^2
//   y_i  = ((x_i - mean) / sqrt(var + eps)) * gamma_i + beta_i
//
// 本实现支持两种参数形态：
// 1) 向量仿射参数 gamma/beta（长度 C）
// 2) 标量仿射参数 gamma_s/beta_s（便于快速验证与某些简化场景）
//
// 优化策略：
// - 每个 block 负责一行；
// - float 路径支持 float4 向量化（条件满足时）；
// - half 路径在 float 中累积均值和方差，兼顾稳定性。
// =========================================================

template <typename T, int BlockSize>
__global__ void layer_norm_rowwise_scalar_kernel(const T* x, const T* gamma, const T* beta, T* y, int rows, int cols, float eps)
{
    using AccT = AccumType_t<T>;

    const int row = static_cast<int>(blockIdx.x);
    if (row >= rows) {
        return;
    }

    const int tid  = static_cast<int>(threadIdx.x);
    const int base = row * cols;

    AccT local_sum = static_cast<AccT>(0);
    for (int c = tid; c < cols; c += BlockSize) {
        local_sum += to_accum(x[base + c]);
    }
    const AccT row_sum = reduce_sum<AccT, BlockSize>(local_sum);
    const AccT mean    = row_sum / static_cast<AccT>(cols);

    AccT local_var = static_cast<AccT>(0);
    for (int c = tid; c < cols; c += BlockSize) {
        const AccT v  = to_accum(x[base + c]) - mean;
        local_var    += v * v;
    }
    const AccT row_var = reduce_sum<AccT, BlockSize>(local_var) / static_cast<AccT>(cols);
    const AccT inv_std = rsqrtf(row_var + static_cast<AccT>(eps));

    for (int c = tid; c < cols; c += BlockSize) {
        const AccT xv  = to_accum(x[base + c]);
        const AccT gv  = to_accum(gamma[c]);
        const AccT bv  = to_accum(beta[c]);
        const AccT out = (xv - mean) * inv_std * gv + bv;
        y[base + c]    = from_accum<T>(out);
    }
}

template <int BlockSize>
__global__ void layer_norm_rowwise_f32_vec4_kernel(const float* x, const float* gamma, const float* beta, float* y, int rows, int cols, float eps)
{
    using Pack4 = Pack<float, 4>;

    const int row = static_cast<int>(blockIdx.x);
    if (row >= rows) {
        return;
    }

    const int tid       = static_cast<int>(threadIdx.x);
    const int base      = row * cols;
    const int pack_cols = cols / 4;

    float local_sum = 0.0f;
    for (int p = tid; p < pack_cols; p += BlockSize) {
        const Pack4 xv  = load_pack<Pack4>(x + base, p);
        local_sum      += xv.data[0] + xv.data[1] + xv.data[2] + xv.data[3];
    }
    const float row_sum = reduce_sum<float, BlockSize>(local_sum);
    const float mean    = row_sum / static_cast<float>(cols);

    float local_var = 0.0f;
    for (int p = tid; p < pack_cols; p += BlockSize) {
        const Pack4 xv  = load_pack<Pack4>(x + base, p);
        const float d0  = xv.data[0] - mean;
        const float d1  = xv.data[1] - mean;
        const float d2  = xv.data[2] - mean;
        const float d3  = xv.data[3] - mean;
        local_var      += d0 * d0 + d1 * d1 + d2 * d2 + d3 * d3;
    }
    const float row_var = reduce_sum<float, BlockSize>(local_var) / static_cast<float>(cols);
    const float inv_std = rsqrtf(row_var + eps);

    for (int p = tid; p < pack_cols; p += BlockSize) {
        const Pack4 xv = load_pack<Pack4>(x + base, p);
        const Pack4 gv = load_pack<Pack4>(gamma, p);
        const Pack4 bv = load_pack<Pack4>(beta, p);

        Pack4 out{};
        out.data[0] = (xv.data[0] - mean) * inv_std * gv.data[0] + bv.data[0];
        out.data[1] = (xv.data[1] - mean) * inv_std * gv.data[1] + bv.data[1];
        out.data[2] = (xv.data[2] - mean) * inv_std * gv.data[2] + bv.data[2];
        out.data[3] = (xv.data[3] - mean) * inv_std * gv.data[3] + bv.data[3];

        store_pack<Pack4>(y + base, p, out);
    }
}

template <typename T>
__global__ void layer_norm_affine_scalar_kernel(const T* x, T gamma_s, T beta_s, T* y, int rows, int cols, float eps)
{
    using AccT = AccumType_t<T>;

    constexpr int BlockSize = 256;
    const int row           = static_cast<int>(blockIdx.x);
    if (row >= rows) {
        return;
    }

    const int tid  = static_cast<int>(threadIdx.x);
    const int base = row * cols;

    AccT local_sum = static_cast<AccT>(0);
    for (int c = tid; c < cols; c += BlockSize) {
        local_sum += to_accum(x[base + c]);
    }
    const AccT row_sum = reduce_sum<AccT, BlockSize>(local_sum);
    const AccT mean    = row_sum / static_cast<AccT>(cols);

    AccT local_var = static_cast<AccT>(0);
    for (int c = tid; c < cols; c += BlockSize) {
        const AccT v  = to_accum(x[base + c]) - mean;
        local_var    += v * v;
    }
    const AccT row_var = reduce_sum<AccT, BlockSize>(local_var) / static_cast<AccT>(cols);
    const AccT inv_std = rsqrtf(row_var + static_cast<AccT>(eps));

    const AccT gs = to_accum(gamma_s);
    const AccT bs = to_accum(beta_s);
    for (int c = tid; c < cols; c += BlockSize) {
        const AccT xv  = to_accum(x[base + c]);
        const AccT out = (xv - mean) * inv_std * gs + bs;
        y[base + c]    = from_accum<T>(out);
    }
}

/**
 * @brief 行级 LayerNorm（向量 gamma/beta）。
 */
template <typename T>
cudaError_t launch_layer_norm(const T* x, const T* gamma, const T* beta, T* y, int rows, int cols, float eps = 1e-5f, cudaStream_t stream = nullptr)
{
    if (rows <= 0 || cols <= 0) {
        return cudaSuccess;
    }

    constexpr int kBlock = 256;
    const dim3 grid(static_cast<unsigned>(rows), 1, 1);
    const dim3 block(static_cast<unsigned>(kBlock), 1, 1);

    bool use_vec = false;
    {
        using Pack4        = Pack<float, 4>;
        const bool can_vec = std::is_same_v<T, float> && (cols % 4) == 0 && can_vectorize_ptr<Pack4>(x) && can_vectorize_ptr<Pack4>(y) &&
                             can_vectorize_ptr<Pack4>(gamma) && can_vectorize_ptr<Pack4>(beta);
        const bool can_advanced = kBuildHasAdvancedPath && check_sm_level(8); // sm >= 8
        // 具备高阶能力时，Auto 在更小列宽也启用向量化；否则只在中等以上列宽启用。
        use_vec = can_vec && (can_advanced || cols >= 64);
    }

    if (use_vec) {
        if constexpr (std::is_same_v<T, float>) {
            layer_norm_rowwise_f32_vec4_kernel<kBlock><<<grid, block, 0, stream>>>(x, gamma, beta, y, rows, cols, eps);
            return cudaGetLastError();
        }
    }

    layer_norm_rowwise_scalar_kernel<T, kBlock><<<grid, block, 0, stream>>>(x, gamma, beta, y, rows, cols, eps);
    return cudaGetLastError();
}

/**
 * @brief 行级 LayerNorm（标量 gamma/beta）。
 */
template <typename T>
cudaError_t launch_layer_norm_affine_scalar(const T* x, T gamma_s, T beta_s, T* y, int rows, int cols, float eps = 1e-5f,
                                            cudaStream_t stream = nullptr)
{
    if (rows <= 0 || cols <= 0) {
        return cudaSuccess;
    }

    constexpr int kBlock = 256;
    const dim3 grid(static_cast<unsigned>(rows), 1, 1);
    const dim3 block(static_cast<unsigned>(kBlock), 1, 1);

    layer_norm_affine_scalar_kernel<T><<<grid, block, 0, stream>>>(x, gamma_s, beta_s, y, rows, cols, eps);
    return cudaGetLastError();
}

} // namespace bee::cuda
