/**
 * @File DeviceView/Vectorize.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 向量化 Load/Store 辅助（float4 / __half2 等）。
 *
 * 用于 ElementWise 等 memory-bound kernel 的连续快路径。
 * 只做按对齐要求的直接 load/store；地址对齐检查由调用方负责。
 */

#pragma once

#include <cuda_runtime.h>

#include <cstdint>

namespace bee::cuda
{

// 某类型一次向量化读写多少个元素（沿用 128-bit 向量宽度作为上限）。
template <typename T>
struct VecWidth
{
    static constexpr int value = 16 / static_cast<int>(sizeof(T));
};

// 向量化 load：一次读取 16 字节。ptr 必须 16B 对齐。
template <typename T>
__device__ __forceinline__ void vec_load16(const T* ptr, T (&out)[16 / sizeof(T)])
{
    using Vec = uint4;
    static_assert(sizeof(Vec) == 16, "uint4 must be 16 bytes");
    const Vec v                  = *reinterpret_cast<const Vec*>(ptr);
    *reinterpret_cast<Vec*>(out) = v;
}

// 向量化 store：一次写入 16 字节。ptr 必须 16B 对齐。
template <typename T>
__device__ __forceinline__ void vec_store16(T* ptr, const T (&in)[16 / sizeof(T)])
{
    using Vec = uint4;
    static_assert(sizeof(Vec) == 16, "uint4 must be 16 bytes");
    const Vec v                  = *reinterpret_cast<const Vec*>(in);
    *reinterpret_cast<Vec*>(ptr) = v;
}

// 地址是否按 N 字节对齐。
template <int N>
__host__ __device__ __forceinline__ bool is_aligned(const void* ptr) noexcept
{
    return (reinterpret_cast<std::uintptr_t>(ptr) & (static_cast<std::uintptr_t>(N) - 1u)) == 0u;
}

} // namespace bee::cuda
