/**
 * @File DeviceView/TensorView.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Device 端 POD shape/stride 视图，传给 kernel 的只读元数据。
 *
 * 设计要点：
 *  - 纯 POD（无构造/析构），栈上按值拷贝进 kernel；
 *  - 不依赖 Tensor 组件，完全独立；
 *  - Rank 固定为模板参数，避免运行期分支；
 *  - data 指针是 device 指针，host 侧只做元数据赋值。
 */

#pragma once

#include <cuda_runtime.h>

#include <cstdint>

namespace bee::cuda
{

inline constexpr int kMaxTensorRank = 8;

// 只读 TensorView：shape + stride（元素为单位）+ data 指针。
// 不管理生命周期；仅作为 kernel 参数使用。
template <typename T, int Rank, typename Idx = std::int64_t>
struct TensorView
{
    static_assert(Rank >= 0 && Rank <= kMaxTensorRank, "Rank out of range");

    T*  data      = nullptr;
    Idx shape[Rank > 0 ? Rank : 1]  = {};
    Idx stride[Rank > 0 ? Rank : 1] = {};

    __host__ __device__ [[nodiscard]] Idx numel() const noexcept
    {
        Idx n = 1;
        #pragma unroll
        for (int i = 0; i < Rank; ++i) n *= shape[i];
        return n;
    }

    // 将线性索引 idx（按 row-major 逻辑形状）映射为按 stride 的物理偏移。
    __host__ __device__ [[nodiscard]] Idx offset_linear(Idx idx) const noexcept
    {
        Idx off = 0;
        #pragma unroll
        for (int i = Rank - 1; i >= 0; --i) {
            const Idx d = shape[i];
            const Idx q = idx / d;
            const Idx r = idx - q * d;
            off += r * stride[i];
            idx = q;
        }
        return off;
    }
};

// 连续存储时的快捷 view（只需 data + numel），省掉 shape/stride 遍历。
template <typename T>
struct ContigView
{
    T*             data   = nullptr;
    std::int64_t   numel  = 0;
};

} // namespace bee::cuda
