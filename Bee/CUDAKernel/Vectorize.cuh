/**
 * @File Vectorize.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace bee::cuda
{

// =========================================================
// 向量化访存工具
// =========================================================

__host__ __device__ inline bool is_aligned(const void* ptr, std::size_t alignment)
{
    return (reinterpret_cast<std::uintptr_t>(ptr) & (alignment - 1)) == 0;
}

// 显式向量指针转换工具，便于在调用处明确表达 float4/int4 向量化意图。
__host__ __device__ inline float4* as_float4_ptr(float* ptr)
{
    return reinterpret_cast<float4*>(ptr);
}

__host__ __device__ inline const float4* as_float4_ptr(const float* ptr)
{
    return reinterpret_cast<const float4*>(ptr);
}

__host__ __device__ inline int4* as_int4_ptr(int* ptr)
{
    return reinterpret_cast<int4*>(ptr);
}

__host__ __device__ inline const int4* as_int4_ptr(const int* ptr)
{
    return reinterpret_cast<const int4*>(ptr);
}

template <typename VecT, typename ScalarT>
__device__ __forceinline__ VecT load_aligned(const ScalarT* ptr, int vec_index)
{
    static_assert(sizeof(VecT) % sizeof(ScalarT) == 0, "VecT must be a pack of ScalarT");
    return reinterpret_cast<const VecT*>(ptr)[vec_index];
}

template <typename VecT, typename ScalarT>
__device__ __forceinline__ void store_aligned(ScalarT* ptr, int vec_index, const VecT& value)
{
    static_assert(sizeof(VecT) % sizeof(ScalarT) == 0, "VecT must be a pack of ScalarT");
    reinterpret_cast<VecT*>(ptr)[vec_index] = value;
}

template <typename T>
__device__ __forceinline__ T load_or_zero(const T* ptr, int idx, int n)
{
    return idx < n ? ptr[idx] : static_cast<T>(0);
}

template <typename T>
__device__ __forceinline__ void store_if_in_bound(T* ptr, int idx, int n, T value)
{
    if (idx < n) {
        ptr[idx] = value;
    }
}

template <int VecWidth, typename T>
__device__ __forceinline__ void load_tail_safe(const T* src, int base_idx, int n, T (&dst)[VecWidth])
{
#pragma unroll
    for (int i = 0; i < VecWidth; ++i) {
        dst[i] = load_or_zero(src, base_idx + i, n);
    }
}

template <int VecWidth, typename T>
__device__ __forceinline__ void store_tail_safe(T* dst, int base_idx, int n, const T (&src)[VecWidth])
{
#pragma unroll
    for (int i = 0; i < VecWidth; ++i) {
        store_if_in_bound(dst, base_idx + i, n, src[i]);
    }
}

// =========================================================
// 向量化基础类型与 Traits
// =========================================================

/**
 * @brief 通用向量包类型，表示由 N 个标量 T 组成的连续向量。
 *
 * 设计目标：
 * 1) 让向量化逻辑不依赖 float2/float4 等固定类型；
 * 2) 兼容任意算子在 pack 内逐元素展开；
 * 3) 明确对齐要求，便于安全 reinterpret。
 */
template <typename T, int N>
struct alignas(sizeof(T) * N) Pack
{
    static_assert(N > 0, "Pack width must be positive");
    T data[N];
};

template <typename PackT>
struct PackTraits;

template <typename T, int N>
struct PackTraits<Pack<T, N>>
{
    using scalar_type                   = T;
    static constexpr int kWidth         = N;
    static constexpr std::size_t kBytes = sizeof(Pack<T, N>);
};

template <typename PackT>
using PackScalarT = PackTraits<PackT>::scalar_type;

template <typename PackT>
inline constexpr int PackWidthV = PackTraits<PackT>::kWidth;

// =========================================================
// 对齐与可向量化检查
// =========================================================

/**
 * @brief 检查单个指针是否可按 PackT 进行向量化访问。
 */
template <typename PackT, typename ScalarT>
__host__ __device__ bool can_vectorize_ptr(const ScalarT* ptr)
{
    static_assert(sizeof(PackT) % sizeof(ScalarT) == 0, "PackT must be composed of ScalarT elements");
    return is_aligned(ptr, alignof(PackT));
}

/**
 * @brief 检查指针组和长度是否适合按 PackT 向量化。
 *
 * 要求：
 * 1) 所有参与指针满足 PackT 对齐；
 * 2) n 至少能覆盖一个完整 pack。
 */
template <typename PackT, typename ScalarT>
__host__ __device__ bool can_vectorize(const ScalarT* ptr, std::size_t n)
{
    return can_vectorize_ptr<PackT>(ptr) && (n >= static_cast<std::size_t>(PackWidthV<PackT>));
}

// =========================================================
// Pack 读写
// =========================================================

/**
 * @brief 从标量数组按 pack index 读取一个 PackT。
 */
template <typename PackT, typename ScalarT>
__device__ __forceinline__ PackT load_pack(const ScalarT* ptr, int pack_index)
{
    static_assert(sizeof(PackT) % sizeof(ScalarT) == 0, "PackT must be composed of ScalarT elements");
    return reinterpret_cast<const PackT*>(ptr)[pack_index];
}

/**
 * @brief 向标量数组按 pack index 写入一个 PackT。
 */
template <typename PackT, typename ScalarT>
__device__ __forceinline__ void store_pack(ScalarT* ptr, int pack_index, const PackT& value)
{
    static_assert(sizeof(PackT) % sizeof(ScalarT) == 0, "PackT must be composed of ScalarT elements");
    reinterpret_cast<PackT*>(ptr)[pack_index] = value;
}

/**
 * @brief 安全读取尾部不足一个 pack 的元素，不足部分补 0。
 */
template <typename PackT, typename ScalarT>
__device__ __forceinline__ PackT load_pack_tail(const ScalarT* ptr, int base_idx, int n)
{
    PackT out{};
#pragma unroll
    for (int i = 0; i < PackWidthV<PackT>; ++i) {
        const int idx = base_idx + i;
        out.data[i]   = idx < n ? ptr[idx] : static_cast<ScalarT>(0);
    }
    return out;
}

/**
 * @brief 安全写回尾部不足一个 pack 的元素，仅写有效区间。
 */
template <typename PackT, typename ScalarT>
__device__ __forceinline__ void store_pack_tail(ScalarT* ptr, int base_idx, int n, const PackT& value)
{
#pragma unroll
    for (int i = 0; i < PackWidthV<PackT>; ++i) {
        const int idx = base_idx + i;
        if (idx < n) {
            ptr[idx] = value.data[i];
        }
    }
}

// =========================================================
// Pack 逐元素变换（Unary/Binary/Ternary）
// =========================================================

/**
 * @brief 对 pack 执行一元逐元素计算。
 */
template <typename PackT, typename Op>
__device__ __forceinline__ PackT map_unary(const PackT& a, Op op)
{
    PackT out{};
#pragma unroll
    for (int i = 0; i < PackWidthV<PackT>; ++i) {
        out.data[i] = op(a.data[i]);
    }
    return out;
}

/**
 * @brief 对 pack 执行二元逐元素计算。
 */
template <typename PackT, typename Op>
__device__ __forceinline__ PackT map_binary(const PackT& a, const PackT& b, Op op)
{
    PackT out{};
#pragma unroll
    for (int i = 0; i < PackWidthV<PackT>; ++i) {
        out.data[i] = op(a.data[i], b.data[i]);
    }
    return out;
}

/**
 * @brief 对 pack 执行三元逐元素计算。
 */
template <typename PackT, typename Op>
__device__ __forceinline__ PackT map_ternary(const PackT& a, const PackT& b, const PackT& c, Op op)
{
    PackT out{};
#pragma unroll
    for (int i = 0; i < PackWidthV<PackT>; ++i) {
        out.data[i] = op(a.data[i], b.data[i], c.data[i]);
    }
    return out;
}

} // namespace bee::cuda
