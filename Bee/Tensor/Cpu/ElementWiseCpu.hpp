#pragma once

// CPU 元素级算子内核：fast-path（连续 SIMD）与 slow-path（广播 stride 迭代）

#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Shape.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Cpu/Simd/Simd.hpp"

#include <cmath>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace bee::cpu
{

// ─────────────────────────────────────────────────────────────────────────────
// SIMD 可用性 trait：避免依赖不完整的 SimdBackend 特化
// 默认 false；为已知的 (类型, ISA) 组合显式设为 true
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, typename ISA> inline constexpr bool kSimdAdd = false;
template <typename T, typename ISA> inline constexpr bool kSimdSub = false;
template <typename T, typename ISA> inline constexpr bool kSimdMul = false;
template <typename T, typename ISA> inline constexpr bool kSimdDiv = false;
template <typename T, typename ISA> inline constexpr bool kSimdNeg = false;
template <typename T, typename ISA> inline constexpr bool kSimdAbs = false;
template <typename T, typename ISA> inline constexpr bool kSimdSqrt = false;
template <typename T, typename ISA> inline constexpr bool kSimdExp  = false;
template <typename T, typename ISA> inline constexpr bool kSimdLog  = false;

// ── IsaScalar 特化 ───────────────────────────────────────────────────────────
template <> inline constexpr bool kSimdAdd<float,    simd::IsaScalar> = true;
template <> inline constexpr bool kSimdAdd<double,   simd::IsaScalar> = true;
template <> inline constexpr bool kSimdAdd<int32_t,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdAdd<int64_t,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdAdd<uint8_t,  simd::IsaScalar> = true;

template <> inline constexpr bool kSimdSub<float,    simd::IsaScalar> = true;
template <> inline constexpr bool kSimdSub<double,   simd::IsaScalar> = true;
template <> inline constexpr bool kSimdSub<int32_t,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdSub<int64_t,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdSub<uint8_t,  simd::IsaScalar> = true;

template <> inline constexpr bool kSimdMul<float,    simd::IsaScalar> = true;
template <> inline constexpr bool kSimdMul<double,   simd::IsaScalar> = true;
template <> inline constexpr bool kSimdMul<int32_t,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdMul<int64_t,  simd::IsaScalar> = true;

template <> inline constexpr bool kSimdDiv<float,    simd::IsaScalar> = true;
template <> inline constexpr bool kSimdDiv<double,   simd::IsaScalar> = true;
template <> inline constexpr bool kSimdDiv<int32_t,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdDiv<int64_t,  simd::IsaScalar> = true;

template <> inline constexpr bool kSimdNeg<float,    simd::IsaScalar> = true;
template <> inline constexpr bool kSimdNeg<double,   simd::IsaScalar> = true;
template <> inline constexpr bool kSimdNeg<int32_t,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdNeg<int64_t,  simd::IsaScalar> = true;

template <> inline constexpr bool kSimdAbs<float,    simd::IsaScalar> = true;
template <> inline constexpr bool kSimdAbs<double,   simd::IsaScalar> = true;
template <> inline constexpr bool kSimdAbs<int32_t,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdAbs<int64_t,  simd::IsaScalar> = true;

template <> inline constexpr bool kSimdSqrt<float,   simd::IsaScalar> = true;
template <> inline constexpr bool kSimdSqrt<double,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdExp<float,    simd::IsaScalar> = true;
template <> inline constexpr bool kSimdExp<double,   simd::IsaScalar> = true;
template <> inline constexpr bool kSimdLog<float,    simd::IsaScalar> = true;
template <> inline constexpr bool kSimdLog<double,   simd::IsaScalar> = true;

// ── IsaAvx2 特化 ─────────────────────────────────────────────────────────────
#ifdef BEE_TENSOR_SIMD_AVX2
template <> inline constexpr bool kSimdAdd<float,    simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdAdd<double,   simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdAdd<int32_t,  simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdAdd<int64_t,  simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdAdd<uint8_t,  simd::IsaAvx2> = true;

template <> inline constexpr bool kSimdSub<float,    simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdSub<double,   simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdSub<int32_t,  simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdSub<int64_t,  simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdSub<uint8_t,  simd::IsaAvx2> = true;

template <> inline constexpr bool kSimdMul<float,    simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdMul<double,   simd::IsaAvx2> = true;

template <> inline constexpr bool kSimdDiv<float,    simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdDiv<double,   simd::IsaAvx2> = true;

template <> inline constexpr bool kSimdNeg<float,    simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdNeg<double,   simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdNeg<int32_t,  simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdNeg<int64_t,  simd::IsaAvx2> = true;

template <> inline constexpr bool kSimdAbs<float,    simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdAbs<double,   simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdAbs<int32_t,  simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdAbs<int64_t,  simd::IsaAvx2> = true;

template <> inline constexpr bool kSimdSqrt<float,   simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdSqrt<double,  simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdExp<float,    simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdExp<double,   simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdLog<float,    simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdLog<double,   simd::IsaAvx2> = true;
#endif

// ─────────────────────────────────────────────────────────────────────────────
// 二元算子标签
// ─────────────────────────────────────────────────────────────────────────────

struct OpAdd
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdAdd<T, ISA>;

    template <typename T>
    static auto scalar(T a, T b) noexcept -> T { return static_cast<T>(a + b); }

    template <typename T, typename ISA>
    static auto simd_apply(
        typename simd::SimdBackend<T, ISA>::reg a,
        typename simd::SimdBackend<T, ISA>::reg b) noexcept
        -> typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::add(a, b);
    }
};

struct OpSub
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdSub<T, ISA>;

    template <typename T>
    static auto scalar(T a, T b) noexcept -> T { return static_cast<T>(a - b); }

    template <typename T, typename ISA>
    static auto simd_apply(
        typename simd::SimdBackend<T, ISA>::reg a,
        typename simd::SimdBackend<T, ISA>::reg b) noexcept
        -> typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::sub(a, b);
    }
};

struct OpMul
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdMul<T, ISA>;

    template <typename T>
    static auto scalar(T a, T b) noexcept -> T { return static_cast<T>(a * b); }

    template <typename T, typename ISA>
    static auto simd_apply(
        typename simd::SimdBackend<T, ISA>::reg a,
        typename simd::SimdBackend<T, ISA>::reg b) noexcept
        -> typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::mul(a, b);
    }
};

struct OpDiv
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdDiv<T, ISA>;

    template <typename T>
    static auto scalar(T a, T b) noexcept -> T { return static_cast<T>(a / b); }

    template <typename T, typename ISA>
    static auto simd_apply(
        typename simd::SimdBackend<T, ISA>::reg a,
        typename simd::SimdBackend<T, ISA>::reg b) noexcept
        -> typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::div(a, b);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 一元算子标签
// ─────────────────────────────────────────────────────────────────────────────

struct OpNeg
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdNeg<T, ISA>;

    template <typename T>
    static auto scalar(T a) noexcept -> T { return static_cast<T>(-a); }

    template <typename T, typename ISA>
    static auto simd_apply(typename simd::SimdBackend<T, ISA>::reg a) noexcept
        -> typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::neg(a);
    }
};

struct OpAbs
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdAbs<T, ISA>;

    template <typename T>
    static auto scalar(T a) noexcept -> T
    {
        if constexpr (std::is_signed_v<T>)
            return a < T{0} ? static_cast<T>(-a) : a;
        else
            return a;
    }

    template <typename T, typename ISA>
    static auto simd_apply(typename simd::SimdBackend<T, ISA>::reg a) noexcept
        -> typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::abs(a);
    }
};

struct OpSqrt
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdSqrt<T, ISA>;

    template <typename T>
    static auto scalar(T a) noexcept -> T { return static_cast<T>(std::sqrt(static_cast<double>(a))); }

    template <typename T, typename ISA>
    static auto simd_apply(typename simd::SimdBackend<T, ISA>::reg a) noexcept
        -> typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::sqrt(a);
    }
};

struct OpExp
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdExp<T, ISA>;

    template <typename T>
    static auto scalar(T a) noexcept -> T { return static_cast<T>(std::exp(static_cast<double>(a))); }

    template <typename T, typename ISA>
    static auto simd_apply(typename simd::SimdBackend<T, ISA>::reg a) noexcept
        -> typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::exp(a);
    }
};

struct OpLog
{
    template <typename T, typename ISA>
    static constexpr bool has_simd = kSimdLog<T, ISA>;

    template <typename T>
    static auto scalar(T a) noexcept -> T { return static_cast<T>(std::log(static_cast<double>(a))); }

    template <typename T, typename ISA>
    static auto simd_apply(typename simd::SimdBackend<T, ISA>::reg a) noexcept
        -> typename simd::SimdBackend<T, ISA>::reg
    {
        return simd::SimdBackend<T, ISA>::log(a);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 连续线性 fast-path
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, typename ISA, typename Op>
auto cpu_binary_linear(int64_t n, const T* a, const T* b, T* out) -> void
{
    using B = simd::SimdBackend<T, ISA>;
    if constexpr (Op::template has_simd<T, ISA>) {
        constexpr auto W = static_cast<int64_t>(B::width);
        int64_t i = 0;
        for (; i + W <= n; i += W) {
            B::storeu(out + i,
                      Op::template simd_apply<T, ISA>(B::loadu(a + i), B::loadu(b + i)));
        }
        for (; i < n; ++i)
            out[i] = Op::template scalar<T>(a[i], b[i]);
    } else {
        for (int64_t i = 0; i < n; ++i)
            out[i] = Op::template scalar<T>(a[i], b[i]);
    }
}

template <typename T, typename ISA, typename Op>
auto cpu_unary_linear(int64_t n, const T* a, T* out) -> void
{
    using B = simd::SimdBackend<T, ISA>;
    if constexpr (Op::template has_simd<T, ISA>) {
        constexpr auto W = static_cast<int64_t>(B::width);
        int64_t i = 0;
        for (; i + W <= n; i += W) {
            B::storeu(out + i, Op::template simd_apply<T, ISA>(B::loadu(a + i)));
        }
        for (; i < n; ++i)
            out[i] = Op::template scalar<T>(a[i]);
    } else {
        for (int64_t i = 0; i < n; ++i)
            out[i] = Op::template scalar<T>(a[i]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 广播 slow-path
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, typename Op>
auto cpu_binary_strided(
    int64_t        ndim,
    const int64_t* out_shape,
    const int64_t* bstrides_a,
    const int64_t* bstrides_b,
    const int64_t* out_strides,
    const T*       a_ptr,
    const T*       b_ptr,
    T*             out_ptr) -> void
{
    int64_t total = 1;
    for (int64_t d = 0; d < ndim; ++d)
        total *= out_shape[d];

    std::vector<int64_t> idx(static_cast<std::size_t>(ndim), 0);
    for (int64_t k = 0; k < total; ++k) {
        int64_t off_a = 0, off_b = 0, off_out = 0;
        for (int64_t d = 0; d < ndim; ++d) {
            off_a   += idx[static_cast<std::size_t>(d)] * bstrides_a[d];
            off_b   += idx[static_cast<std::size_t>(d)] * bstrides_b[d];
            off_out += idx[static_cast<std::size_t>(d)] * out_strides[d];
        }
        out_ptr[off_out] = Op::template scalar<T>(a_ptr[off_a], b_ptr[off_b]);

        for (int64_t d = ndim - 1; d >= 0; --d) {
            ++idx[static_cast<std::size_t>(d)];
            if (idx[static_cast<std::size_t>(d)] < out_shape[d])
                break;
            idx[static_cast<std::size_t>(d)] = 0;
        }
    }
}

template <typename T, typename Op>
auto cpu_unary_strided(
    int64_t        ndim,
    const int64_t* shape,
    const int64_t* strides_a,
    const int64_t* out_strides,
    const T*       a_ptr,
    T*             out_ptr) -> void
{
    int64_t total = 1;
    for (int64_t d = 0; d < ndim; ++d)
        total *= shape[d];

    std::vector<int64_t> idx(static_cast<std::size_t>(ndim), 0);
    for (int64_t k = 0; k < total; ++k) {
        int64_t off_a = 0, off_out = 0;
        for (int64_t d = 0; d < ndim; ++d) {
            off_a   += idx[static_cast<std::size_t>(d)] * strides_a[d];
            off_out += idx[static_cast<std::size_t>(d)] * out_strides[d];
        }
        out_ptr[off_out] = Op::template scalar<T>(a_ptr[off_a]);

        for (int64_t d = ndim - 1; d >= 0; --d) {
            ++idx[static_cast<std::size_t>(d)];
            if (idx[static_cast<std::size_t>(d)] < shape[d])
                break;
            idx[static_cast<std::size_t>(d)] = 0;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Tensor 级分派
// ─────────────────────────────────────────────────────────────────────────────

inline auto make_broadcast_strides(
    const Shape&   in_shape,
    const Strides& in_strides,
    int64_t        ndim,
    const Shape&   out_shape) -> std::vector<int64_t>
{
    const int64_t r_in = static_cast<int64_t>(in_shape.size());
    std::vector<int64_t> bst(static_cast<std::size_t>(ndim), 0);
    for (int64_t d = 0; d < ndim; ++d) {
        const int64_t id     = d - (ndim - r_in);
        const int64_t dim_in = (id >= 0) ? in_shape[static_cast<std::size_t>(id)] : 1;
        const int64_t st_in  = (id >= 0) ? in_strides[static_cast<std::size_t>(id)] : 0;
        bst[static_cast<std::size_t>(d)] =
            (dim_in == 1 && out_shape[static_cast<std::size_t>(d)] > 1) ? 0 : st_in;
    }
    return bst;
}

template <typename T, typename Op>
auto cpu_elementwise_binary(const Tensor& a, const Tensor& b, Tensor& out) -> void
{
    using ISA = simd::DefaultIsa;

    const int64_t n    = out.numel();
    const int64_t ndim = out.ndim();

    const auto* a_ptr   = static_cast<const T*>(a.data_ptr());
    const auto* b_ptr   = static_cast<const T*>(b.data_ptr());
    auto*       out_ptr = static_cast<T*>(out.data_ptr());

    if (a.is_contiguous() && b.is_contiguous() &&
        a.shape() == out.shape() && b.shape() == out.shape())
    {
        cpu_binary_linear<T, ISA, Op>(n, a_ptr, b_ptr, out_ptr);
        return;
    }

    const auto  bst_a = make_broadcast_strides(a.shape(), a.strides(), ndim, out.shape());
    const auto  bst_b = make_broadcast_strides(b.shape(), b.strides(), ndim, out.shape());
    const auto& osh   = out.shape();
    const auto& ost   = out.strides();

    cpu_binary_strided<T, Op>(
        ndim,
        osh.data(),
        bst_a.data(),
        bst_b.data(),
        ost.data(),
        a_ptr, b_ptr, out_ptr);
}

template <typename T, typename Op>
auto cpu_elementwise_unary(const Tensor& a, Tensor& out) -> void
{
    using ISA = simd::DefaultIsa;

    const int64_t n    = out.numel();
    const int64_t ndim = out.ndim();

    const auto* a_ptr   = static_cast<const T*>(a.data_ptr());
    auto*       out_ptr = static_cast<T*>(out.data_ptr());

    if (a.is_contiguous() && a.shape() == out.shape()) {
        cpu_unary_linear<T, ISA, Op>(n, a_ptr, out_ptr);
        return;
    }

    const auto& ast = a.strides();
    const auto& osh = out.shape();
    const auto& ost = out.strides();

    cpu_unary_strided<T, Op>(
        ndim,
        osh.data(),
        ast.data(),
        ost.data(),
        a_ptr, out_ptr);
}

} // namespace bee::cpu
