/**
 * @File Elementwise.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Launch.cuh"
#include "Vectorize.cuh"

#include <cuda_runtime.h>

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace bee::cuda
{

// =========================================================
// Concept 约束：限制一元/二元/三元操作符形状
// =========================================================

/**
 * @brief 一元算子约束：op(x) 可调用且返回值可转换到 Out。
 */
template <typename Op, typename Out, typename In>
concept UnaryOp = std::is_trivially_copyable_v<Op> && requires(Op op, In x) {
    { op(x) } -> std::convertible_to<Out>;
};

/**
 * @brief 二元算子约束：op(x, y) 可调用且返回值可转换到 Out。
 */
template <typename Op, typename Out, typename In0, typename In1>
concept BinaryOp = std::is_trivially_copyable_v<Op> && requires(Op op, In0 x, In1 y) {
    { op(x, y) } -> std::convertible_to<Out>;
};

/**
 * @brief 三元算子约束：op(x, y, z) 可调用且返回值可转换到 Out。
 */
template <typename Op, typename Out, typename In0, typename In1, typename In2>
concept TernaryOp = std::is_trivially_copyable_v<Op> && requires(Op op, In0 x, In1 y, In2 z) {
    { op(x, y, z) } -> std::convertible_to<Out>;
};

// =========================================================
// 基础 Grid-Stride Kernel（标量路径）
// =========================================================

template <typename Out, typename In, typename Op>
    requires UnaryOp<Op, Out, In>
__global__ void unary_kernel_scalar(Out* out, const In* in, int n, Op op)
{
    const int idx    = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int stride = static_cast<int>(blockDim.x * gridDim.x);

    for (int i = idx; i < n; i += stride) {
        out[i] = static_cast<Out>(op(in[i]));
    }
}

template <typename Out, typename In0, typename In1, typename Op>
    requires BinaryOp<Op, Out, In0, In1>
__global__ void binary_kernel_scalar(Out* out, const In0* in0, const In1* in1, int n, Op op)
{
    const int idx    = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int stride = static_cast<int>(blockDim.x * gridDim.x);

    for (int i = idx; i < n; i += stride) {
        out[i] = static_cast<Out>(op(in0[i], in1[i]));
    }
}

template <typename Out, typename In0, typename In1, typename In2, typename Op>
    requires TernaryOp<Op, Out, In0, In1, In2>
__global__ void ternary_kernel_scalar(Out* out, const In0* in0, const In1* in1, const In2* in2, int n, Op op)
{
    const int idx    = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int stride = static_cast<int>(blockDim.x * gridDim.x);

    for (int i = idx; i < n; i += stride) {
        out[i] = static_cast<Out>(op(in0[i], in1[i], in2[i]));
    }
}

// =========================================================
// 向量化 Kernel，同类型
// =========================================================

template <typename T, int VecWidth, typename Op>
    requires UnaryOp<Op, T, T>
__global__ void unary_kernel_vectorized(T* out, const T* in, int n, Op op)
{
    using PackT = Pack<T, VecWidth>;

    const int pack_n = n / VecWidth;
    const int idx    = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int stride = static_cast<int>(blockDim.x * gridDim.x);

    for (int p = idx; p < pack_n; p += stride) {
        const PackT a = load_pack<PackT>(in, p);
        const PackT v = map_unary<PackT>(a, op);
        store_pack<PackT>(out, p, v);
    }

    const int tail_base = pack_n * VecWidth;
    for (int i = tail_base + idx; i < n; i += stride) {
        out[i] = static_cast<T>(op(in[i]));
    }
}

template <typename T, int VecWidth, typename Op>
    requires BinaryOp<Op, T, T, T>
__global__ void binary_kernel_vectorized(T* out, const T* in0, const T* in1, int n, Op op)
{
    using PackT = Pack<T, VecWidth>;

    const int pack_n = n / VecWidth;
    const int idx    = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int stride = static_cast<int>(blockDim.x * gridDim.x);

    for (int p = idx; p < pack_n; p += stride) {
        const PackT a = load_pack<PackT>(in0, p);
        const PackT b = load_pack<PackT>(in1, p);
        const PackT v = map_binary<PackT>(a, b, op);
        store_pack<PackT>(out, p, v);
    }

    const int tail_base = pack_n * VecWidth;
    for (int i = tail_base + idx; i < n; i += stride) {
        out[i] = static_cast<T>(op(in0[i], in1[i]));
    }
}

template <typename T, int VecWidth, typename Op>
    requires TernaryOp<Op, T, T, T, T>
__global__ void ternary_kernel_vectorized(T* out, const T* in0, const T* in1, const T* in2, int n, Op op)
{
    using PackT = Pack<T, VecWidth>;

    const int pack_n = n / VecWidth;
    const int idx    = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int stride = static_cast<int>(blockDim.x * gridDim.x);

    for (int p = idx; p < pack_n; p += stride) {
        const PackT a = load_pack<PackT>(in0, p);
        const PackT b = load_pack<PackT>(in1, p);
        const PackT c = load_pack<PackT>(in2, p);
        const PackT v = map_ternary<PackT>(a, b, c, op);
        store_pack<PackT>(out, p, v);
    }

    const int tail_base = pack_n * VecWidth;
    for (int i = tail_base + idx; i < n; i += stride) {
        out[i] = static_cast<T>(op(in0[i], in1[i], in2[i]));
    }
}

// =========================================================
// Launch 封装：自动选择向量化或标量路径
// =========================================================

/**
 * @brief 一元逐元素 launch。
 *
 * 第一版策略：
 * 1) 当 Out/In 同类型、指针对齐满足 Pack 要求时，走向量化路径；
 * 2) 否则走标量路径；
 * 3) 始终支持任意 n（含尾部不足 pack 的情况）。
 */
template <int VecWidth = 4, typename Out, typename In, typename Op>
    requires UnaryOp<Op, Out, In>
cudaError_t launch_elementwise_unary(Out* out, const In* in, int n, Op op, cudaStream_t stream = nullptr)
{
    if (n <= 0) {
        return cudaSuccess;
    }

    constexpr int kBlock = 256;
    const auto cfg       = make_1d(n, kBlock);

    if constexpr (std::is_same_v<Out, In>) {
        using PackT = Pack<Out, VecWidth>;
        if (can_vectorize<PackT>(out, static_cast<std::size_t>(n)) && can_vectorize<PackT>(in, static_cast<std::size_t>(n))) {
            unary_kernel_vectorized<Out, VecWidth, Op><<<cfg.grid, cfg.block, 0, stream>>>(out, in, n, op);
            return cudaGetLastError();
        }
    }

    unary_kernel_scalar<Out, In, Op><<<cfg.grid, cfg.block, 0, stream>>>(out, in, n, op);
    return cudaGetLastError();
}

template <int VecWidth = 4, typename Out, typename In0, typename In1, typename Op>
    requires BinaryOp<Op, Out, In0, In1>
cudaError_t launch_elementwise_binary(Out* out, const In0* in0, const In1* in1, int n, Op op, cudaStream_t stream = nullptr)
{
    if (n <= 0) {
        return cudaSuccess;
    }

    constexpr int kBlock = 256;
    const auto cfg       = make_1d(n, kBlock);

    if constexpr (std::is_same_v<Out, In0> && std::is_same_v<Out, In1>) {
        using PackT = Pack<Out, VecWidth>;
        if (can_vectorize<PackT>(out, static_cast<std::size_t>(n)) && can_vectorize<PackT>(in0, static_cast<std::size_t>(n)) &&
            can_vectorize<PackT>(in1, static_cast<std::size_t>(n))) {
            binary_kernel_vectorized<Out, VecWidth, Op><<<cfg.grid, cfg.block, 0, stream>>>(out, in0, in1, n, op);
            return cudaGetLastError();
        }
    }

    binary_kernel_scalar<Out, In0, In1, Op><<<cfg.grid, cfg.block, 0, stream>>>(out, in0, in1, n, op);
    return cudaGetLastError();
}

template <int VecWidth = 4, typename Out, typename In0, typename In1, typename In2, typename Op>
    requires TernaryOp<Op, Out, In0, In1, In2>
cudaError_t launch_elementwise_ternary(Out* out, const In0* in0, const In1* in1, const In2* in2, int n, Op op, cudaStream_t stream = nullptr)
{
    if (n <= 0) {
        return cudaSuccess;
    }

    constexpr int kBlock = 256;
    const auto cfg       = make_1d(n, kBlock);

    if constexpr (std::is_same_v<Out, In0> && std::is_same_v<Out, In1> && std::is_same_v<Out, In2>) {
        using PackT = Pack<Out, VecWidth>;
        if (can_vectorize<PackT>(out, static_cast<std::size_t>(n)) && can_vectorize<PackT>(in0, static_cast<std::size_t>(n)) &&
            can_vectorize<PackT>(in1, static_cast<std::size_t>(n)) && can_vectorize<PackT>(in2, static_cast<std::size_t>(n))) {
            ternary_kernel_vectorized<Out, VecWidth, Op><<<cfg.grid, cfg.block, 0, stream>>>(out, in0, in1, in2, n, op);
            return cudaGetLastError();
        }
    }

    ternary_kernel_scalar<Out, In0, In1, In2, Op><<<cfg.grid, cfg.block, 0, stream>>>(out, in0, in1, in2, n, op);
    return cudaGetLastError();
}

} // namespace bee::cuda
