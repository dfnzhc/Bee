#pragma once

#include "SIMD/Traits.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace bee::simd
{

// -----------------------------------------------------------------------
// float 标量后端
// -----------------------------------------------------------------------
template <>
struct SimdBackend<float, IsaScalar>
{
    static constexpr std::size_t width = 1;
    using reg                          = float;

    // clang-format off
    static auto load(const float* p) -> reg { return *p; }
    static auto loadu(const float* p) -> reg { return *p; }
    static auto store(float* p, reg v) -> void { *p = v; }
    static auto storeu(float* p, reg v) -> void { *p = v; }
    static auto set1(float x) -> reg { return x; }

    static auto add(reg a, reg b) -> reg { return a + b; }
    static auto sub(reg a, reg b) -> reg { return a - b; }
    static auto mul(reg a, reg b) -> reg { return a * b; }
    static auto div(reg a, reg b) -> reg { return a / b; }

    static auto min(reg a, reg b) -> reg { return std::min(a, b); }
    static auto max(reg a, reg b) -> reg { return std::max(a, b); }
    static auto neg(reg a) -> reg { return -a; }
    static auto abs(reg a) -> reg { return std::abs(a); }
    static auto sqrt(reg a) -> reg { return std::sqrt(a); }
    static auto exp(reg a) -> reg { return std::exp(a); }
    static auto log(reg a) -> reg { return std::log(a); }

    static auto reduce_sum(reg v) -> float { return v; }
    static auto reduce_min(reg v) -> float { return v; }
    static auto reduce_max(reg v) -> float { return v; }
    // clang-format on
};

// -----------------------------------------------------------------------
// double 标量后端
// -----------------------------------------------------------------------
template <>
struct SimdBackend<double, IsaScalar>
{
    static constexpr std::size_t width = 1;
    using reg                          = double;

    // clang-format off
    static auto load(const double* p) -> reg { return *p; }
    static auto loadu(const double* p) -> reg { return *p; }
    static auto store(double* p, reg v) -> void { *p = v; }
    static auto storeu(double* p, reg v) -> void { *p = v; }
    static auto set1(double x) -> reg { return x; }

    static auto add(reg a, reg b) -> reg { return a + b; }
    static auto sub(reg a, reg b) -> reg { return a - b; }
    static auto mul(reg a, reg b) -> reg { return a * b; }
    static auto div(reg a, reg b) -> reg { return a / b; }

    static auto min(reg a, reg b) -> reg { return std::min(a, b); }
    static auto max(reg a, reg b) -> reg { return std::max(a, b); }
    static auto neg(reg a) -> reg { return -a; }
    static auto abs(reg a) -> reg { return std::abs(a); }
    static auto sqrt(reg a) -> reg { return std::sqrt(a); }
    static auto exp(reg a) -> reg { return std::exp(a); }
    static auto log(reg a) -> reg { return std::log(a); }

    static auto reduce_sum(reg v) -> double { return v; }
    static auto reduce_min(reg v) -> double { return v; }
    static auto reduce_max(reg v) -> double { return v; }
    // clang-format on
};

// -----------------------------------------------------------------------
// int32_t 标量后端
// -----------------------------------------------------------------------
template <>
struct SimdBackend<int32_t, IsaScalar>
{
    static constexpr std::size_t width = 1;
    using reg                          = int32_t;

    // clang-format off
    static auto load(const int32_t* p) -> reg { return *p; }
    static auto loadu(const int32_t* p) -> reg { return *p; }
    static auto store(int32_t* p, reg v) -> void { *p = v; }
    static auto storeu(int32_t* p, reg v) -> void { *p = v; }
    static auto set1(int32_t x) -> reg { return x; }

    static auto add(reg a, reg b) -> reg { return a + b; }
    static auto sub(reg a, reg b) -> reg { return a - b; }
    static auto mul(reg a, reg b) -> reg { return a * b; }
    static auto div(reg a, reg b) -> reg { return a / b; }

    static auto min(reg a, reg b) -> reg { return std::min(a, b); }
    static auto max(reg a, reg b) -> reg { return std::max(a, b); }
    static auto neg(reg a) -> reg { return -a; }
    static auto abs(reg a) -> reg { return std::abs(a); }

    static auto reduce_sum(reg v) -> int32_t { return v; }
    static auto reduce_min(reg v) -> int32_t { return v; }
    static auto reduce_max(reg v) -> int32_t { return v; }
    // clang-format on
};

// -----------------------------------------------------------------------
// int64_t 标量后端
// -----------------------------------------------------------------------
template <>
struct SimdBackend<int64_t, IsaScalar>
{
    static constexpr std::size_t width = 1;
    using reg                          = int64_t;

    // clang-format off
    static auto load(const int64_t* p) -> reg { return *p; }
    static auto loadu(const int64_t* p) -> reg { return *p; }
    static auto store(int64_t* p, reg v) -> void { *p = v; }
    static auto storeu(int64_t* p, reg v) -> void { *p = v; }
    static auto set1(int64_t x) -> reg { return x; }

    static auto add(reg a, reg b) -> reg { return a + b; }
    static auto sub(reg a, reg b) -> reg { return a - b; }
    static auto mul(reg a, reg b) -> reg { return a * b; }
    static auto div(reg a, reg b) -> reg { return a / b; }

    static auto min(reg a, reg b) -> reg { return std::min(a, b); }
    static auto max(reg a, reg b) -> reg { return std::max(a, b); }
    static auto neg(reg a) -> reg { return -a; }
    static auto abs(reg a) -> reg { return std::abs(a); }

    static auto reduce_sum(reg v) -> int64_t { return v; }
    static auto reduce_min(reg v) -> int64_t { return v; }
    static auto reduce_max(reg v) -> int64_t { return v; }
    // clang-format on
};

// -----------------------------------------------------------------------
// uint8_t 标量后端
// 注意：不提供 neg/abs/mul/div（避免无符号溢出争议）
// -----------------------------------------------------------------------
template <>
struct SimdBackend<uint8_t, IsaScalar>
{
    static constexpr std::size_t width = 1;
    using reg                          = uint8_t;

    // clang-format off
    static auto load(const uint8_t* p)    -> reg  { return *p; }
    static auto loadu(const uint8_t* p)   -> reg  { return *p; }
    static auto store(uint8_t* p, reg v)  -> void { *p = v; }
    static auto storeu(uint8_t* p, reg v) -> void { *p = v; }
    static auto set1(uint8_t x)           -> reg  { return x; }

    static auto add(reg a, reg b) -> reg { return static_cast<uint8_t>(a + b); }
    static auto sub(reg a, reg b) -> reg { return static_cast<uint8_t>(a - b); }

    static auto min(reg a, reg b) -> reg { return std::min(a, b); }
    static auto max(reg a, reg b) -> reg { return std::max(a, b); }

    static auto reduce_sum(reg v) -> uint8_t { return v; }
    static auto reduce_min(reg v) -> uint8_t { return v; }
    static auto reduce_max(reg v) -> uint8_t { return v; }
    // clang-format on
};

} // namespace bee::simd
