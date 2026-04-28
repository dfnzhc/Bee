#pragma once

// CPU Reduce 算子内核：全局 reduce（SIMD 快路径 + 标量慢路径）
// 与按轴 reduce（连续快速路径 + 通用步长慢速路径）
// 全局 reduce 使用 4 路 SIMD 累加器打破浮点依赖链，并在大输入上启用 parallel_for。

#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Shape.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "SIMD/SIMD.hpp"
#include "Base/Parallel/ParallelFor.hpp"

#include <climits>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <mutex>
#include <type_traits>
#include <vector>

namespace bee::cpu
{

// ─────────────────────────────────────────────────────────────────────────────
// SIMD reduce 可用性 trait：默认 false，对已知 (T, ISA) 组合显式设为 true
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, typename ISA>
inline constexpr bool kSimdReduceSum = false;
template <typename T, typename ISA>
inline constexpr bool kSimdReduceMin = false;
template <typename T, typename ISA>
inline constexpr bool kSimdReduceMax = false;

// ── IsaScalar 特化：标量后端对所有支持类型均有 reduce 原语 ───────────────────

template <>
inline constexpr bool kSimdReduceSum<float, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceSum<double, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceSum<int32_t, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceSum<int64_t, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceSum<uint8_t, simd::IsaScalar> = true;

template <>
inline constexpr bool kSimdReduceMin<float, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceMin<double, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceMin<int32_t, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceMin<int64_t, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceMin<uint8_t, simd::IsaScalar> = true;

template <>
inline constexpr bool kSimdReduceMax<float, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceMax<double, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceMax<int32_t, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceMax<int64_t, simd::IsaScalar> = true;
template <>
inline constexpr bool kSimdReduceMax<uint8_t, simd::IsaScalar> = true;

// ── IsaAvx2 特化：AVX2 后端的 reduce 能力与类型对应关系 ─────────────────────
// reduce_sum：float/double/i32/i64/u8 均有；reduce_min/max：仅 float/double/i32

#ifdef BEE_SIMD_ENABLE_AVX2
template <>
inline constexpr bool kSimdReduceSum<float, simd::IsaAvx2> = true;
template <>
inline constexpr bool kSimdReduceSum<double, simd::IsaAvx2> = true;
template <>
inline constexpr bool kSimdReduceSum<int32_t, simd::IsaAvx2> = true;
template <>
inline constexpr bool kSimdReduceSum<int64_t, simd::IsaAvx2> = true;
template <>
inline constexpr bool kSimdReduceSum<uint8_t, simd::IsaAvx2> = true;

// AVX2 的 i64 缺少有符号比较原语，u8 缺少 reduce_min/reduce_max：均不启用
template <>
inline constexpr bool kSimdReduceMin<float, simd::IsaAvx2> = true;
template <>
inline constexpr bool kSimdReduceMin<double, simd::IsaAvx2> = true;
template <>
inline constexpr bool kSimdReduceMin<int32_t, simd::IsaAvx2> = true;

template <>
inline constexpr bool kSimdReduceMax<float, simd::IsaAvx2> = true;
template <>
inline constexpr bool kSimdReduceMax<double, simd::IsaAvx2> = true;
template <>
inline constexpr bool kSimdReduceMax<int32_t, simd::IsaAvx2> = true;
#endif

// ── IsaSse2 特化：SSE2 后端的 reduce 能力 ────────────────────────────────────
// reduce_sum：5 种类型均有（u8 用 sad_epu8，i64 用标量提取）
// reduce_min/max：float/double/int32/uint8 有；i64 无有符号 SSE2 比较指令，跳过

#ifdef BEE_SIMD_ENABLE_SSE2
template <>
inline constexpr bool kSimdReduceSum<float, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceSum<double, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceSum<int32_t, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceSum<int64_t, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceSum<uint8_t, simd::IsaSse2> = true;

template <>
inline constexpr bool kSimdReduceMin<float, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceMin<double, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceMin<int32_t, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceMin<uint8_t, simd::IsaSse2> = true;

template <>
inline constexpr bool kSimdReduceMax<float, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceMax<double, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceMax<int32_t, simd::IsaSse2> = true;
template <>
inline constexpr bool kSimdReduceMax<uint8_t, simd::IsaSse2> = true;
#endif

// ── IsaAvx512 特化：AVX-512 后端的 reduce 能力 ──────────────────────────────
// i64 min/max/abs 是 AVX-512F 原生（不同于 AVX2）
// u8 依赖 AVX512BW，用 __AVX512BW__ 守卫

#ifdef BEE_SIMD_ENABLE_AVX512
template <>
inline constexpr bool kSimdReduceSum<float, simd::IsaAvx512> = true;
template <>
inline constexpr bool kSimdReduceSum<double, simd::IsaAvx512> = true;
template <>
inline constexpr bool kSimdReduceSum<int32_t, simd::IsaAvx512> = true;
template <>
inline constexpr bool kSimdReduceSum<int64_t, simd::IsaAvx512> = true;
    #ifdef __AVX512BW__
template <>
inline constexpr bool kSimdReduceSum<uint8_t, simd::IsaAvx512> = true;
    #endif

template <>
inline constexpr bool kSimdReduceMin<float, simd::IsaAvx512> = true;
template <>
inline constexpr bool kSimdReduceMin<double, simd::IsaAvx512> = true;
template <>
inline constexpr bool kSimdReduceMin<int32_t, simd::IsaAvx512> = true;
template <>
inline constexpr bool kSimdReduceMin<int64_t, simd::IsaAvx512> = true;

template <>
inline constexpr bool kSimdReduceMax<float, simd::IsaAvx512> = true;
template <>
inline constexpr bool kSimdReduceMax<double, simd::IsaAvx512> = true;
template <>
inline constexpr bool kSimdReduceMax<int32_t, simd::IsaAvx512> = true;
template <>
inline constexpr bool kSimdReduceMax<int64_t, simd::IsaAvx512> = true;
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Reduce 算子标签：封装恒等元、标量合并与 SIMD 接口
// ─────────────────────────────────────────────────────────────────────────────

struct OpReduceSum
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdReduceSum<T, ISA>;

    // 加法恒等元为 0
    template <typename T>
    static auto identity() -> T
    {
        return T{0};
    }

    template <typename T>
    static auto scalar(T a, T b) -> T
    {
        return static_cast<T>(a + b);
    }

    template <typename T, typename ISA>
    static auto simd_acc(typename simd::SimdBackend<T, ISA>::reg a, typename simd::SimdBackend<T, ISA>::reg b) ->
        typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::add(a, b);
    }

    template <typename T, typename ISA>
    static auto simd_reduce(typename simd::SimdBackend<T, ISA>::reg v) -> T
    {
        return simd::SimdBackend<T, ISA>::reduce_sum(v);
    }
};

struct OpReduceMin
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdReduceMin<T, ISA>;

    // min 恒等元为类型最大值
    template <typename T>
    static auto identity() -> T
    {
        return std::numeric_limits<T>::max();
    }

    template <typename T>
    static auto scalar(T a, T b) -> T
    {
        return a < b ? a : b;
    }

    template <typename T, typename ISA>
    static auto simd_acc(typename simd::SimdBackend<T, ISA>::reg a, typename simd::SimdBackend<T, ISA>::reg b) ->
        typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::min(a, b);
    }

    template <typename T, typename ISA>
    static auto simd_reduce(typename simd::SimdBackend<T, ISA>::reg v) -> T
    {
        return simd::SimdBackend<T, ISA>::reduce_min(v);
    }
};

struct OpReduceMax
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdReduceMax<T, ISA>;

    // max 恒等元为类型最小值
    template <typename T>
    static auto identity() -> T
    {
        return std::numeric_limits<T>::lowest();
    }

    template <typename T>
    static auto scalar(T a, T b) -> T
    {
        return a > b ? a : b;
    }

    template <typename T, typename ISA>
    static auto simd_acc(typename simd::SimdBackend<T, ISA>::reg a, typename simd::SimdBackend<T, ISA>::reg b) ->
        typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::max(a, b);
    }

    template <typename T, typename ISA>
    static auto simd_reduce(typename simd::SimdBackend<T, ISA>::reg v) -> T
    {
        return simd::SimdBackend<T, ISA>::reduce_max(v);
    }
};

// prod 无 SIMD 原语，全部走标量路径
struct OpReduceProd
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = false;

    // 乘法恒等元为 1
    template <typename T>
    static auto identity() -> T
    {
        return T{1};
    }

    template <typename T>
    static auto scalar(T a, T b) -> T
    {
        return static_cast<T>(a * b);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 全局 reduce：连续 SIMD fast-path（4 路累加器打破 FP 依赖链）
// ─────────────────────────────────────────────────────────────────────────────

// 对 n 个连续元素执行 reduce，支持 SIMD 加速（若 Op 支持该类型/ISA）
template <typename T, typename ISA, typename Op>
auto cpu_global_reduce_linear(int64_t n, const T* ptr) -> T
{
    using B = simd::SimdBackend<T, ISA>;

    if constexpr (Op::template has_simd<T, ISA>) {
        constexpr auto W = static_cast<int64_t>(B::width);
        if (n >= 4 * W) {
            // 4 路独立累加器，每轮吃 4*W 个元素；打破 FP add 的依赖链
            auto    a0 = B::set1(Op::template identity<T>());
            auto    a1 = a0;
            auto    a2 = a0;
            auto    a3 = a0;
            int64_t i  = 0;
            for (; i + 4 * W <= n; i += 4 * W) {
                a0 = Op::template simd_acc<T, ISA>(a0, B::loadu(ptr + i + 0 * W));
                a1 = Op::template simd_acc<T, ISA>(a1, B::loadu(ptr + i + 1 * W));
                a2 = Op::template simd_acc<T, ISA>(a2, B::loadu(ptr + i + 2 * W));
                a3 = Op::template simd_acc<T, ISA>(a3, B::loadu(ptr + i + 3 * W));
            }
            // 处理余下 [0..3] 个 SIMD 宽度块
            auto acc = Op::template simd_acc<T, ISA>(Op::template simd_acc<T, ISA>(a0, a1), Op::template simd_acc<T, ISA>(a2, a3));
            for (; i + W <= n; i += W)
                acc = Op::template simd_acc<T, ISA>(acc, B::loadu(ptr + i));
            T result = Op::template simd_reduce<T, ISA>(acc);
            // 处理尾部不足一 SIMD 宽度的元素
            for (; i < n; ++i)
                result = Op::template scalar<T>(result, ptr[i]);
            return result;
        }
        if (n >= W) {
            auto    acc = B::set1(Op::template identity<T>());
            int64_t i   = 0;
            for (; i + W <= n; i += W)
                acc = Op::template simd_acc<T, ISA>(acc, B::loadu(ptr + i));
            T result = Op::template simd_reduce<T, ISA>(acc);
            for (; i < n; ++i)
                result = Op::template scalar<T>(result, ptr[i]);
            return result;
        }
    }

    // 纯标量路径
    T result = Op::template identity<T>();
    for (int64_t i = 0; i < n; ++i)
        result = Op::template scalar<T>(result, ptr[i]);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// 全局 reduce：非连续 stride 迭代 slow-path
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, typename Op>
auto cpu_global_reduce_strided(const Tensor& a) -> T
{
    const int64_t ndim    = a.ndim();
    const auto&   shape   = a.shape();
    const auto&   strides = a.strides();
    const auto*   ptr     = static_cast<const T*>(a.data_ptr());
    const int64_t n       = a.numel();

    T result = Op::template identity<T>();

    if (ndim == 0) {
        // 0-rank 标量直接读取单元素
        return Op::template scalar<T>(result, ptr[0]);
    }

    std::vector<int64_t> idx(static_cast<std::size_t>(ndim), 0);
    for (int64_t k = 0; k < n; ++k) {
        int64_t off = 0;
        for (int64_t d = 0; d < ndim; ++d)
            off += idx[static_cast<std::size_t>(d)] * strides[static_cast<std::size_t>(d)];
        result = Op::template scalar<T>(result, ptr[off]);

        // 推进多维索引（末尾维度最快）
        for (int64_t d = ndim - 1; d >= 0; --d) {
            ++idx[static_cast<std::size_t>(d)];
            if (idx[static_cast<std::size_t>(d)] < shape[static_cast<std::size_t>(d)])
                break;
            idx[static_cast<std::size_t>(d)] = 0;
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// 全局 reduce：dispatch（连续 → parallel SIMD；非连续 → stride 迭代）
// ─────────────────────────────────────────────────────────────────────────────

// 并行阈值：按输入字节量判断是否并行（小于 L2 的数据保留单线程，避免派发开销）。
// 13700k：P-core L2=2MB；single-thread reduce 在 1MB 数据上就能接近 LLC 带宽峰值。
inline constexpr int64_t kReduceParallelBytes = 4 * 1024 * 1024;
// 每 worker 分块字节数（64 KB ~ L2 命中 + 并行收益的平衡点）。
inline constexpr int64_t kReduceChunkBytes = 64 * 1024;

// 将 reduce 结果写入 0-rank 输出张量
template <typename T, typename ISA, typename Op>
auto cpu_reduce_global_dispatch(const Tensor& a, Tensor& out) -> void
{
    const auto*   in_ptr  = static_cast<const T*>(a.data_ptr());
    auto*         out_ptr = static_cast<T*>(out.data_ptr());
    const int64_t n       = a.numel();
    const int64_t bytes   = n * static_cast<int64_t>(sizeof(T));

    T result;
    if (!a.is_contiguous()) {
        result = cpu_global_reduce_strided<T, Op>(a);
    } else if (bytes < kReduceParallelBytes) {
        result = cpu_global_reduce_linear<T, ISA, Op>(n, in_ptr);
    } else {
        const std::size_t grain = static_cast<std::size_t>(std::max<int64_t>(1, kReduceChunkBytes / static_cast<int64_t>(sizeof(T))));
        // 工人局部 partial，最终主线程做 N 路合并。
        std::vector<T> partials;
        std::mutex     mu;
        parallel::parallel_for(std::size_t{0}, static_cast<std::size_t>(n), grain, [&](std::size_t lo, std::size_t hi) {
            T               p = cpu_global_reduce_linear<T, ISA, Op>(static_cast<int64_t>(hi - lo), in_ptr + lo);
            std::lock_guard lk(mu);
            partials.push_back(p);
        });
        result = Op::template identity<T>();
        for (const T& p : partials)
            result = Op::template scalar<T>(result, p);
    }

    out_ptr[0] = result;
}

// ─────────────────────────────────────────────────────────────────────────────
// mean 全局 reduce 的 double 累加特化（供 I32/I64 输入使用）
// ─────────────────────────────────────────────────────────────────────────────

// Tin 为输入类型（I32/I64 等整型），Tout 为输出类型（F64）
template <typename Tin, typename Tout>
auto cpu_reduce_mean_global_dispatch(const Tensor& a, Tensor& out) -> void
{
    const auto*   in_ptr  = static_cast<const Tin*>(a.data_ptr());
    auto*         out_ptr = static_cast<Tout*>(out.data_ptr());
    const int64_t n       = a.numel();

    double acc = 0.0;
    if (a.is_contiguous()) {
        for (int64_t i = 0; i < n; ++i)
            acc += static_cast<double>(in_ptr[i]);
    } else {
        // 非连续：通过 stride 迭代
        const int64_t ndim    = a.ndim();
        const auto&   shape   = a.shape();
        const auto&   strides = a.strides();
        if (ndim == 0) {
            acc = static_cast<double>(in_ptr[0]);
        } else {
            std::vector<int64_t> idx(static_cast<std::size_t>(ndim), 0);
            for (int64_t k = 0; k < n; ++k) {
                int64_t off = 0;
                for (int64_t d = 0; d < ndim; ++d)
                    off += idx[static_cast<std::size_t>(d)] * strides[static_cast<std::size_t>(d)];
                acc += static_cast<double>(in_ptr[off]);
                for (int64_t d = ndim - 1; d >= 0; --d) {
                    ++idx[static_cast<std::size_t>(d)];
                    if (idx[static_cast<std::size_t>(d)] < shape[static_cast<std::size_t>(d)])
                        break;
                    idx[static_cast<std::size_t>(d)] = 0;
                }
            }
        }
    }
    out_ptr[0] = static_cast<Tout>(acc / static_cast<double>(n));
}

// ─────────────────────────────────────────────────────────────────────────────
// 按轴 reduce：朴素实现，outer / K / inner 三层循环
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, typename Op>
auto cpu_reduce_axis_dispatch(const Tensor& a, int64_t dim, bool keepdim, Tensor& out) -> void
{
    const int64_t ndim      = a.ndim();
    const auto&   shape     = a.shape();
    const auto&   strides_a = a.strides();
    const auto*   in_ptr    = static_cast<const T*>(a.data_ptr());
    auto*         out_ptr   = static_cast<T*>(out.data_ptr());

    const int64_t K = shape[static_cast<std::size_t>(dim)];

    // 计算外维和内维大小
    int64_t outer = 1;
    for (int64_t d = 0; d < dim; ++d)
        outer *= shape[static_cast<std::size_t>(d)];

    int64_t inner = 1;
    for (int64_t d = dim + 1; d < ndim; ++d)
        inner *= shape[static_cast<std::size_t>(d)];

    if (a.is_contiguous()) {
        // 连续快速路径：直接用下标公式访问
        for (int64_t o = 0; o < outer; ++o) {
            for (int64_t i = 0; i < inner; ++i) {
                T acc = Op::template identity<T>();
                for (int64_t k = 0; k < K; ++k)
                    acc = Op::template scalar<T>(acc, in_ptr[o * K * inner + k * inner + i]);
                out_ptr[o * inner + i] = acc;
            }
        }
    } else {
        // 通用步长慢速路径：用多维索引计算外部偏移和内部偏移
        const int64_t outer_ndim = dim;
        const int64_t inner_ndim = ndim - dim - 1;

        std::vector<int64_t> outer_idx(static_cast<std::size_t>(outer_ndim), 0);
        std::vector<int64_t> inner_idx(static_cast<std::size_t>(inner_ndim), 0);

        for (int64_t o = 0; o < outer; ++o) {
            // 计算当前 outer 多维索引对应的输入偏移
            int64_t outer_off = 0;
            for (int64_t d = 0; d < outer_ndim; ++d)
                outer_off += outer_idx[static_cast<std::size_t>(d)] * strides_a[static_cast<std::size_t>(d)];

            for (int64_t i = 0; i < inner; ++i) {
                // 计算当前 inner 多维索引对应的输入偏移
                int64_t inner_off = 0;
                for (int64_t d = 0; d < inner_ndim; ++d)
                    inner_off += inner_idx[static_cast<std::size_t>(d)] * strides_a[static_cast<std::size_t>(dim + 1 + d)];

                const int64_t in_base = outer_off + inner_off;

                T acc = Op::template identity<T>();
                for (int64_t k = 0; k < K; ++k)
                    acc = Op::template scalar<T>(acc, in_ptr[in_base + k * strides_a[static_cast<std::size_t>(dim)]]);
                out_ptr[o * inner + i] = acc;

                // 推进 inner 多维索引
                for (int64_t d = inner_ndim - 1; d >= 0; --d) {
                    ++inner_idx[static_cast<std::size_t>(d)];
                    if (inner_idx[static_cast<std::size_t>(d)] < shape[static_cast<std::size_t>(dim + 1 + d)])
                        break;
                    inner_idx[static_cast<std::size_t>(d)] = 0;
                }
            }

            // 推进 outer 多维索引
            for (int64_t d = outer_ndim - 1; d >= 0; --d) {
                ++outer_idx[static_cast<std::size_t>(d)];
                if (outer_idx[static_cast<std::size_t>(d)] < shape[static_cast<std::size_t>(d)])
                    break;
                outer_idx[static_cast<std::size_t>(d)] = 0;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// mean 按轴 reduce：支持输入类型（Tin）与输出类型（Tout）不同
// ─────────────────────────────────────────────────────────────────────────────

template <typename Tin, typename Tout>
auto cpu_reduce_mean_axis_dispatch(const Tensor& a, int64_t dim, bool keepdim, Tensor& out) -> void
{
    const int64_t ndim      = a.ndim();
    const auto&   shape     = a.shape();
    const auto&   strides_a = a.strides();
    const auto*   in_ptr    = static_cast<const Tin*>(a.data_ptr());
    auto*         out_ptr   = static_cast<Tout*>(out.data_ptr());

    const int64_t K = shape[static_cast<std::size_t>(dim)];

    int64_t outer = 1;
    for (int64_t d = 0; d < dim; ++d)
        outer *= shape[static_cast<std::size_t>(d)];

    int64_t inner = 1;
    for (int64_t d = dim + 1; d < ndim; ++d)
        inner *= shape[static_cast<std::size_t>(d)];

    if (a.is_contiguous()) {
        for (int64_t o = 0; o < outer; ++o) {
            for (int64_t i = 0; i < inner; ++i) {
                // 累加为 double 以保证整型输入的精度
                double acc = 0.0;
                for (int64_t k = 0; k < K; ++k)
                    acc += static_cast<double>(in_ptr[o * K * inner + k * inner + i]);
                out_ptr[o * inner + i] = static_cast<Tout>(acc / static_cast<double>(K));
            }
        }
    } else {
        const int64_t outer_ndim = dim;
        const int64_t inner_ndim = ndim - dim - 1;

        std::vector<int64_t> outer_idx(static_cast<std::size_t>(outer_ndim), 0);
        std::vector<int64_t> inner_idx(static_cast<std::size_t>(inner_ndim), 0);

        for (int64_t o = 0; o < outer; ++o) {
            int64_t outer_off = 0;
            for (int64_t d = 0; d < outer_ndim; ++d)
                outer_off += outer_idx[static_cast<std::size_t>(d)] * strides_a[static_cast<std::size_t>(d)];

            for (int64_t i = 0; i < inner; ++i) {
                int64_t inner_off = 0;
                for (int64_t d = 0; d < inner_ndim; ++d)
                    inner_off += inner_idx[static_cast<std::size_t>(d)] * strides_a[static_cast<std::size_t>(dim + 1 + d)];

                const int64_t in_base = outer_off + inner_off;

                double acc = 0.0;
                for (int64_t k = 0; k < K; ++k)
                    acc += static_cast<double>(in_ptr[in_base + k * strides_a[static_cast<std::size_t>(dim)]]);
                out_ptr[o * inner + i] = static_cast<Tout>(acc / static_cast<double>(K));

                for (int64_t d = inner_ndim - 1; d >= 0; --d) {
                    ++inner_idx[static_cast<std::size_t>(d)];
                    if (inner_idx[static_cast<std::size_t>(d)] < shape[static_cast<std::size_t>(dim + 1 + d)])
                        break;
                    inner_idx[static_cast<std::size_t>(d)] = 0;
                }
            }

            for (int64_t d = outer_ndim - 1; d >= 0; --d) {
                ++outer_idx[static_cast<std::size_t>(d)];
                if (outer_idx[static_cast<std::size_t>(d)] < shape[static_cast<std::size_t>(d)])
                    break;
                outer_idx[static_cast<std::size_t>(d)] = 0;
            }
        }
    }
}

} // namespace bee::cpu
