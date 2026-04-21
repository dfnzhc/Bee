#pragma once

#include "SIMD/Traits.hpp"

#ifdef BEE_TENSOR_SIMD_SSE2

#include <immintrin.h>
#include <smmintrin.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace bee::simd
{

// -----------------------------------------------------------------------
// float × SSE2：__m128，宽度 = 4
// -----------------------------------------------------------------------
template <>
struct SimdBackend<float, IsaSse2>
{
    static constexpr std::size_t width = 4;
    using reg = __m128;

    // clang-format off
    static auto load(const float* p)  -> reg  { return _mm_load_ps(p); }
    static auto loadu(const float* p) -> reg  { return _mm_loadu_ps(p); }
    static auto store(float* p, reg v)  -> void { _mm_store_ps(p, v); }
    static auto storeu(float* p, reg v) -> void { _mm_storeu_ps(p, v); }
    static auto set1(float x) -> reg { return _mm_set1_ps(x); }

    static auto add(reg a, reg b) -> reg { return _mm_add_ps(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm_sub_ps(a, b); }
    static auto mul(reg a, reg b) -> reg { return _mm_mul_ps(a, b); }
    static auto div(reg a, reg b) -> reg { return _mm_div_ps(a, b); }

    static auto min(reg a, reg b) -> reg { return _mm_min_ps(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm_max_ps(a, b); }
    // clang-format on

    // 翻转符号位
    static auto neg(reg a) -> reg
    {
        return _mm_xor_ps(a, _mm_set1_ps(-0.0f));
    }

    // 清除符号位取绝对值
    static auto abs(reg a) -> reg
    {
        return _mm_andnot_ps(_mm_set1_ps(-0.0f), a);
    }

    static auto sqrt(reg a) -> reg { return _mm_sqrt_ps(a); }

    // exp 回退标量逐元素计算
    static auto exp(reg v) -> reg
    {
        alignas(16) float buf[4];
        _mm_store_ps(buf, v);
        for (int i = 0; i < 4; ++i)
            buf[i] = std::exp(buf[i]);
        return _mm_load_ps(buf);
    }

    // log 回退标量逐元素计算
    static auto log(reg v) -> reg
    {
        alignas(16) float buf[4];
        _mm_store_ps(buf, v);
        for (int i = 0; i < 4; ++i)
            buf[i] = std::log(buf[i]);
        return _mm_load_ps(buf);
    }

    // 水平求和：4 个 float → 1 个 float
    // 使用 movehl + shuffle，纯 SSE2
    static auto reduce_sum(reg v) -> float
    {
        __m128 t = _mm_add_ps(v, _mm_movehl_ps(v, v));
        t = _mm_add_ss(t, _mm_shuffle_ps(t, t, 1));
        return _mm_cvtss_f32(t);
    }

    // 水平求最小值
    static auto reduce_min(reg v) -> float
    {
        __m128 t = _mm_min_ps(v, _mm_movehl_ps(v, v));
        t = _mm_min_ss(t, _mm_shuffle_ps(t, t, 1));
        return _mm_cvtss_f32(t);
    }

    // 水平求最大值
    static auto reduce_max(reg v) -> float
    {
        __m128 t = _mm_max_ps(v, _mm_movehl_ps(v, v));
        t = _mm_max_ss(t, _mm_shuffle_ps(t, t, 1));
        return _mm_cvtss_f32(t);
    }
};

// -----------------------------------------------------------------------
// double × SSE2：__m128d，宽度 = 2
// -----------------------------------------------------------------------
template <>
struct SimdBackend<double, IsaSse2>
{
    static constexpr std::size_t width = 2;
    using reg = __m128d;

    // clang-format off
    static auto load(const double* p)  -> reg  { return _mm_load_pd(p); }
    static auto loadu(const double* p) -> reg  { return _mm_loadu_pd(p); }
    static auto store(double* p, reg v)  -> void { _mm_store_pd(p, v); }
    static auto storeu(double* p, reg v) -> void { _mm_storeu_pd(p, v); }
    static auto set1(double x) -> reg { return _mm_set1_pd(x); }

    static auto add(reg a, reg b) -> reg { return _mm_add_pd(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm_sub_pd(a, b); }
    static auto mul(reg a, reg b) -> reg { return _mm_mul_pd(a, b); }
    static auto div(reg a, reg b) -> reg { return _mm_div_pd(a, b); }

    static auto min(reg a, reg b) -> reg { return _mm_min_pd(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm_max_pd(a, b); }
    // clang-format on

    static auto neg(reg a) -> reg
    {
        return _mm_xor_pd(a, _mm_set1_pd(-0.0));
    }

    static auto abs(reg a) -> reg
    {
        return _mm_andnot_pd(_mm_set1_pd(-0.0), a);
    }

    static auto sqrt(reg a) -> reg { return _mm_sqrt_pd(a); }

    static auto exp(reg v) -> reg
    {
        alignas(16) double buf[2];
        _mm_store_pd(buf, v);
        buf[0] = std::exp(buf[0]);
        buf[1] = std::exp(buf[1]);
        return _mm_load_pd(buf);
    }

    static auto log(reg v) -> reg
    {
        alignas(16) double buf[2];
        _mm_store_pd(buf, v);
        buf[0] = std::log(buf[0]);
        buf[1] = std::log(buf[1]);
        return _mm_load_pd(buf);
    }

    // 水平求和：2 个 double → 1 个 double
    static auto reduce_sum(reg v) -> double
    {
        __m128d t = _mm_add_sd(v, _mm_unpackhi_pd(v, v));
        return _mm_cvtsd_f64(t);
    }

    // 水平求最小值
    static auto reduce_min(reg v) -> double
    {
        __m128d t = _mm_min_sd(v, _mm_unpackhi_pd(v, v));
        return _mm_cvtsd_f64(t);
    }

    // 水平求最大值
    static auto reduce_max(reg v) -> double
    {
        __m128d t = _mm_max_sd(v, _mm_unpackhi_pd(v, v));
        return _mm_cvtsd_f64(t);
    }
};

// -----------------------------------------------------------------------
// int32_t × SSE2+SSE4.1：__m128i，宽度 = 4
// min/max 依赖 SSE4.1 _mm_min_epi32；abs 用 SSE2 算术右移实现
// -----------------------------------------------------------------------
template <>
struct SimdBackend<int32_t, IsaSse2>
{
    static constexpr std::size_t width = 4;
    using reg = __m128i;

    static auto load(const int32_t* p) -> reg
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(p));
    }
    static auto loadu(const int32_t* p) -> reg
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
    }
    static auto store(int32_t* p, reg v) -> void
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(p), v);
    }
    static auto storeu(int32_t* p, reg v) -> void
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v);
    }
    static auto set1(int32_t x) -> reg { return _mm_set1_epi32(x); }

    static auto add(reg a, reg b) -> reg { return _mm_add_epi32(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm_sub_epi32(a, b); }

    // SSE4.1 有符号 32-bit 比较
    static auto min(reg a, reg b) -> reg { return _mm_min_epi32(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm_max_epi32(a, b); }

    // 取反：0 - v
    static auto neg(reg a) -> reg { return _mm_sub_epi32(_mm_setzero_si128(), a); }

    // 绝对值：SSE2 算术右移模拟（符号掩码法）
    // mask = 算术右移 31 位：负数得 0xFFFFFFFF，非负得 0
    // |x| = (x ^ mask) - mask
    static auto abs(reg a) -> reg
    {
        __m128i mask = _mm_srai_epi32(a, 31);
        return _mm_sub_epi32(_mm_xor_si128(a, mask), mask);
    }

    // 水平求和：4 个 int32 → 1 个 int32（纯 SSE2）
    static auto reduce_sum(reg v) -> int32_t
    {
        __m128i t = _mm_add_epi32(v, _mm_srli_si128(v, 8));
        t = _mm_add_epi32(t, _mm_srli_si128(t, 4));
        return _mm_cvtsi128_si32(t);
    }

    // 水平求最小值（SSE4.1）
    static auto reduce_min(reg v) -> int32_t
    {
        __m128i t = _mm_min_epi32(v, _mm_srli_si128(v, 8));
        t = _mm_min_epi32(t, _mm_srli_si128(t, 4));
        return _mm_cvtsi128_si32(t);
    }

    // 水平求最大值（SSE4.1）
    static auto reduce_max(reg v) -> int32_t
    {
        __m128i t = _mm_max_epi32(v, _mm_srli_si128(v, 8));
        t = _mm_max_epi32(t, _mm_srli_si128(t, 4));
        return _mm_cvtsi128_si32(t);
    }
};

// -----------------------------------------------------------------------
// int64_t × SSE2：__m128i，宽度 = 2
// 仅实装 add/sub/neg/abs/reduce_sum；min/max/reduce_min/reduce_max 缺 SSE2 原语，跳过
// -----------------------------------------------------------------------
template <>
struct SimdBackend<int64_t, IsaSse2>
{
    static constexpr std::size_t width = 2;
    using reg = __m128i;

    static auto load(const int64_t* p) -> reg
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(p));
    }
    static auto loadu(const int64_t* p) -> reg
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
    }
    static auto store(int64_t* p, reg v) -> void
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(p), v);
    }
    static auto storeu(int64_t* p, reg v) -> void
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v);
    }
    static auto set1(int64_t x) -> reg { return _mm_set1_epi64x(x); }

    static auto add(reg a, reg b) -> reg { return _mm_add_epi64(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm_sub_epi64(a, b); }

    // 取反：0 - v
    static auto neg(reg a) -> reg { return _mm_sub_epi64(_mm_setzero_si128(), a); }

    // 绝对值：利用高 32-bit lane 算术右移模拟 64-bit 符号掩码
    // 将每个 64-bit lane 的高 32 位算术右移 31 位，广播到整个 64 位 lane
    // sign = [hi31>>31, hi31>>31] (两个 lane)
    // |x| = (x ^ sign) - sign
    static auto abs(reg a) -> reg
    {
        // 取各 64-bit lane 高 32 位的符号位，广播到低 32 位
        __m128i sign32 = _mm_srai_epi32(_mm_shuffle_epi32(a, 0xF5), 31);
        return _mm_sub_epi64(_mm_xor_si128(a, sign32), sign32);
    }

    // 水平求和：提取两个 64-bit lane 相加
    static auto reduce_sum(reg v) -> int64_t
    {
        alignas(16) int64_t buf[2];
        _mm_store_si128(reinterpret_cast<__m128i*>(buf), v);
        return buf[0] + buf[1];
    }
};

// -----------------------------------------------------------------------
// uint8_t × SSE2：__m128i，宽度 = 16
// min/max 为 SSE2 原生；reduce_min/reduce_max 用 _mm_min_epu8 手工折叠
// -----------------------------------------------------------------------
template <>
struct SimdBackend<uint8_t, IsaSse2>
{
    static constexpr std::size_t width = 16;
    using reg = __m128i;

    static auto load(const uint8_t* p) -> reg
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(p));
    }
    static auto loadu(const uint8_t* p) -> reg
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
    }
    static auto store(uint8_t* p, reg v) -> void
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(p), v);
    }
    static auto storeu(uint8_t* p, reg v) -> void
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v);
    }
    static auto set1(uint8_t x) -> reg { return _mm_set1_epi8(static_cast<char>(x)); }

    // 字节加法（环绕，与标量行为一致）
    static auto add(reg a, reg b) -> reg { return _mm_add_epi8(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm_sub_epi8(a, b); }

    // SSE2 原生无符号字节比较
    static auto min(reg a, reg b) -> reg { return _mm_min_epu8(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm_max_epu8(a, b); }

    // 水平求和：用 _mm_sad_epu8 将 16 个 uint8 分为两组 SAD 求和
    static auto reduce_sum(reg v) -> uint8_t
    {
        __m128i zero = _mm_setzero_si128();
        __m128i sad  = _mm_sad_epu8(v, zero);
        // sad[63:0] = 低 8 字节之和，sad[127:64] = 高 8 字节之和
        __m128i hi   = _mm_srli_si128(sad, 8);
        uint32_t total = static_cast<uint32_t>(_mm_cvtsi128_si32(sad))
                       + static_cast<uint32_t>(_mm_cvtsi128_si32(hi));
        return static_cast<uint8_t>(total);
    }

    // 水平求最小值：逐步折叠
    static auto reduce_min(reg v) -> uint8_t
    {
        __m128i t = _mm_min_epu8(v, _mm_srli_si128(v, 8));
        t = _mm_min_epu8(t, _mm_srli_si128(t, 4));
        t = _mm_min_epu8(t, _mm_srli_si128(t, 2));
        t = _mm_min_epu8(t, _mm_srli_si128(t, 1));
        return static_cast<uint8_t>(_mm_cvtsi128_si32(t) & 0xFF);
    }

    // 水平求最大值：逐步折叠
    static auto reduce_max(reg v) -> uint8_t
    {
        __m128i t = _mm_max_epu8(v, _mm_srli_si128(v, 8));
        t = _mm_max_epu8(t, _mm_srli_si128(t, 4));
        t = _mm_max_epu8(t, _mm_srli_si128(t, 2));
        t = _mm_max_epu8(t, _mm_srli_si128(t, 1));
        return static_cast<uint8_t>(_mm_cvtsi128_si32(t) & 0xFF);
    }
};

} // namespace bee::simd

#endif // BEE_TENSOR_SIMD_SSE2
