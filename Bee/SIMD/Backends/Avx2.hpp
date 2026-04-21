#pragma once

#include "SIMD/Traits.hpp"

#ifdef BEE_SIMD_ENABLE_AVX2

#include <immintrin.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace bee::simd
{

// -----------------------------------------------------------------------
// float × AVX2：__m256，宽度 = 8
// -----------------------------------------------------------------------
template <>
struct SimdBackend<float, IsaAvx2>
{
    static constexpr std::size_t width = 8;
    using reg = __m256;

    // clang-format off
    static auto load(const float* p) -> reg { return _mm256_load_ps(p); }
    static auto loadu(const float* p) -> reg { return _mm256_loadu_ps(p); }
    static auto store(float* p, reg v) -> void { _mm256_store_ps(p, v); }
    static auto storeu(float* p, reg v) -> void { _mm256_storeu_ps(p, v); }
    static auto set1(float x) -> reg { return _mm256_set1_ps(x); }

    static auto add(reg a, reg b) -> reg { return _mm256_add_ps(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm256_sub_ps(a, b); }
    static auto mul(reg a, reg b) -> reg { return _mm256_mul_ps(a, b); }
    static auto div(reg a, reg b) -> reg { return _mm256_div_ps(a, b); }

    static auto min(reg a, reg b) -> reg { return _mm256_min_ps(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm256_max_ps(a, b); }
    // clang-format on

    // 翻转符号位
    static auto neg(reg a) -> reg
    {
        return _mm256_xor_ps(a, _mm256_set1_ps(-0.0f));
    }

    // 清除符号位取绝对值
    static auto abs(reg a) -> reg
    {
        return _mm256_andnot_ps(_mm256_set1_ps(-0.0f), a);
    }

    static auto sqrt(reg a) -> reg { return _mm256_sqrt_ps(a); }

    // exp/log 回退到标量逐元素计算
    static auto exp(reg v) -> reg
    {
        alignas(32) float buf[8];
        _mm256_store_ps(buf, v);
        for (int i = 0; i < 8; ++i)
            buf[i] = std::exp(buf[i]);
        return _mm256_load_ps(buf);
    }

    static auto log(reg v) -> reg
    {
        alignas(32) float buf[8];
        _mm256_store_ps(buf, v);
        for (int i = 0; i < 8; ++i)
            buf[i] = std::log(buf[i]);
        return _mm256_load_ps(buf);
    }

    // 水平求和：8 个 float → 1 个 float
    static auto reduce_sum(reg v) -> float
    {
        __m256 t = _mm256_hadd_ps(v, v);
        t = _mm256_hadd_ps(t, t);
        __m128 lo  = _mm256_castps256_ps128(t);
        __m128 hi  = _mm256_extractf128_ps(t, 1);
        __m128 sum = _mm_add_ss(lo, hi);
        return _mm_cvtss_f32(sum);
    }

    // 水平求最小值
    static auto reduce_min(reg v) -> float
    {
        __m128 lo = _mm256_castps256_ps128(v);
        __m128 hi = _mm256_extractf128_ps(v, 1);
        __m128 mn = _mm_min_ps(lo, hi);
        mn = _mm_min_ps(mn, _mm_movehl_ps(mn, mn));
        mn = _mm_min_ss(mn, _mm_shuffle_ps(mn, mn, 0x01));
        return _mm_cvtss_f32(mn);
    }

    // 水平求最大值
    static auto reduce_max(reg v) -> float
    {
        __m128 lo = _mm256_castps256_ps128(v);
        __m128 hi = _mm256_extractf128_ps(v, 1);
        __m128 mx = _mm_max_ps(lo, hi);
        mx = _mm_max_ps(mx, _mm_movehl_ps(mx, mx));
        mx = _mm_max_ss(mx, _mm_shuffle_ps(mx, mx, 0x01));
        return _mm_cvtss_f32(mx);
    }
};

// -----------------------------------------------------------------------
// double × AVX2：__m256d，宽度 = 4
// -----------------------------------------------------------------------
template <>
struct SimdBackend<double, IsaAvx2>
{
    static constexpr std::size_t width = 4;
    using reg = __m256d;

    // clang-format off
    static auto load(const double* p) -> reg { return _mm256_load_pd(p); }
    static auto loadu(const double* p) -> reg { return _mm256_loadu_pd(p); }
    static auto store(double* p, reg v) -> void { _mm256_store_pd(p, v); }
    static auto storeu(double* p, reg v) -> void { _mm256_storeu_pd(p, v); }
    static auto set1(double x) -> reg { return _mm256_set1_pd(x); }

    static auto add(reg a, reg b) -> reg { return _mm256_add_pd(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm256_sub_pd(a, b); }
    static auto mul(reg a, reg b) -> reg { return _mm256_mul_pd(a, b); }
    static auto div(reg a, reg b) -> reg { return _mm256_div_pd(a, b); }

    static auto min(reg a, reg b) -> reg { return _mm256_min_pd(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm256_max_pd(a, b); }
    // clang-format on

    static auto neg(reg a) -> reg
    {
        return _mm256_xor_pd(a, _mm256_set1_pd(-0.0));
    }

    static auto abs(reg a) -> reg
    {
        return _mm256_andnot_pd(_mm256_set1_pd(-0.0), a);
    }

    static auto sqrt(reg a) -> reg { return _mm256_sqrt_pd(a); }

    static auto exp(reg v) -> reg
    {
        alignas(32) double buf[4];
        _mm256_store_pd(buf, v);
        for (int i = 0; i < 4; ++i)
            buf[i] = std::exp(buf[i]);
        return _mm256_load_pd(buf);
    }

    static auto log(reg v) -> reg
    {
        alignas(32) double buf[4];
        _mm256_store_pd(buf, v);
        for (int i = 0; i < 4; ++i)
            buf[i] = std::log(buf[i]);
        return _mm256_load_pd(buf);
    }

    // 水平求和：4 个 double → 1 个 double
    static auto reduce_sum(reg v) -> double
    {
        __m128d lo   = _mm256_castpd256_pd128(v);
        __m128d hi   = _mm256_extractf128_pd(v, 1);
        __m128d s    = _mm_add_pd(lo, hi);
        __m128d s_hi = _mm_unpackhi_pd(s, s);
        return _mm_cvtsd_f64(_mm_add_sd(s, s_hi));
    }

    // 水平求最小值
    static auto reduce_min(reg v) -> double
    {
        __m128d lo = _mm256_castpd256_pd128(v);
        __m128d hi = _mm256_extractf128_pd(v, 1);
        __m128d mn = _mm_min_pd(lo, hi);
        mn = _mm_min_sd(mn, _mm_unpackhi_pd(mn, mn));
        return _mm_cvtsd_f64(mn);
    }

    // 水平求最大值
    static auto reduce_max(reg v) -> double
    {
        __m128d lo = _mm256_castpd256_pd128(v);
        __m128d hi = _mm256_extractf128_pd(v, 1);
        __m128d mx = _mm_max_pd(lo, hi);
        mx = _mm_max_sd(mx, _mm_unpackhi_pd(mx, mx));
        return _mm_cvtsd_f64(mx);
    }
};

// -----------------------------------------------------------------------
// int32_t × AVX2：__m256i，宽度 = 8
// 不提供 mul/div（256-bit i32 mul 细节繁琐，后续按需补充）
// -----------------------------------------------------------------------
template <>
struct SimdBackend<int32_t, IsaAvx2>
{
    static constexpr std::size_t width = 8;
    using reg = __m256i;

    static auto load(const int32_t* p) -> reg
    {
        return _mm256_load_si256(reinterpret_cast<const __m256i*>(p));
    }
    static auto loadu(const int32_t* p) -> reg
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
    }
    static auto store(int32_t* p, reg v) -> void
    {
        _mm256_store_si256(reinterpret_cast<__m256i*>(p), v);
    }
    static auto storeu(int32_t* p, reg v) -> void
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v);
    }
    static auto set1(int32_t x) -> reg { return _mm256_set1_epi32(x); }

    static auto add(reg a, reg b) -> reg { return _mm256_add_epi32(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm256_sub_epi32(a, b); }

    static auto min(reg a, reg b) -> reg { return _mm256_min_epi32(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm256_max_epi32(a, b); }

    // 取反：0 - v
    static auto neg(reg a) -> reg { return _mm256_sub_epi32(_mm256_setzero_si256(), a); }

    // 绝对值
    static auto abs(reg a) -> reg { return _mm256_abs_epi32(a); }

    // 水平求和：8 个 int32 → 1 个 int32
    static auto reduce_sum(reg v) -> int32_t
    {
        __m128i lo  = _mm256_castsi256_si128(v);
        __m128i hi  = _mm256_extracti128_si256(v, 1);
        __m128i sum = _mm_add_epi32(lo, hi);
        sum = _mm_hadd_epi32(sum, sum);
        sum = _mm_hadd_epi32(sum, sum);
        return _mm_cvtsi128_si32(sum);
    }

    // 水平求最小值
    static auto reduce_min(reg v) -> int32_t
    {
        __m128i lo = _mm256_castsi256_si128(v);
        __m128i hi = _mm256_extracti128_si256(v, 1);
        __m128i mn = _mm_min_epi32(lo, hi);
        mn = _mm_min_epi32(mn, _mm_srli_si128(mn, 8));
        mn = _mm_min_epi32(mn, _mm_srli_si128(mn, 4));
        return _mm_cvtsi128_si32(mn);
    }

    // 水平求最大值
    static auto reduce_max(reg v) -> int32_t
    {
        __m128i lo = _mm256_castsi256_si128(v);
        __m128i hi = _mm256_extracti128_si256(v, 1);
        __m128i mx = _mm_max_epi32(lo, hi);
        mx = _mm_max_epi32(mx, _mm_srli_si128(mx, 8));
        mx = _mm_max_epi32(mx, _mm_srli_si128(mx, 4));
        return _mm_cvtsi128_si32(mx);
    }
};

// -----------------------------------------------------------------------
// int64_t × AVX2：__m256i，宽度 = 4
// 不提供 min/max/reduce_min/reduce_max（AVX2 缺少有符号 64-bit 比较）
// -----------------------------------------------------------------------
template <>
struct SimdBackend<int64_t, IsaAvx2>
{
    static constexpr std::size_t width = 4;
    using reg = __m256i;

    static auto load(const int64_t* p) -> reg
    {
        return _mm256_load_si256(reinterpret_cast<const __m256i*>(p));
    }
    static auto loadu(const int64_t* p) -> reg
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
    }
    static auto store(int64_t* p, reg v) -> void
    {
        _mm256_store_si256(reinterpret_cast<__m256i*>(p), v);
    }
    static auto storeu(int64_t* p, reg v) -> void
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v);
    }
    static auto set1(int64_t x) -> reg { return _mm256_set1_epi64x(x); }

    static auto add(reg a, reg b) -> reg { return _mm256_add_epi64(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm256_sub_epi64(a, b); }

    // 取反：0 - v
    static auto neg(reg a) -> reg { return _mm256_sub_epi64(_mm256_setzero_si256(), a); }

    // 绝对值：AVX2 无 _mm256_abs_epi64，手工用符号掩码实现
    // sign[i] = (v[i] < 0) ? -1 : 0；result = (v ^ sign) - sign
    static auto abs(reg a) -> reg
    {
        __m256i sign = _mm256_cmpgt_epi64(_mm256_setzero_si256(), a);
        return _mm256_sub_epi64(_mm256_xor_si256(a, sign), sign);
    }

    // 水平求和：4 个 int64 → 1 个 int64（回退到标量提取）
    static auto reduce_sum(reg v) -> int64_t
    {
        alignas(32) int64_t buf[4];
        _mm256_store_si256(reinterpret_cast<__m256i*>(buf), v);
        return buf[0] + buf[1] + buf[2] + buf[3];
    }
};

// -----------------------------------------------------------------------
// uint8_t × AVX2：__m256i，宽度 = 32
// 不提供 neg/abs/mul/div
// -----------------------------------------------------------------------
template <>
struct SimdBackend<uint8_t, IsaAvx2>
{
    static constexpr std::size_t width = 32;
    using reg = __m256i;

    static auto load(const uint8_t* p) -> reg
    {
        return _mm256_load_si256(reinterpret_cast<const __m256i*>(p));
    }
    static auto loadu(const uint8_t* p) -> reg
    {
        return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
    }
    static auto store(uint8_t* p, reg v) -> void
    {
        _mm256_store_si256(reinterpret_cast<__m256i*>(p), v);
    }
    static auto storeu(uint8_t* p, reg v) -> void
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v);
    }
    static auto set1(uint8_t x) -> reg { return _mm256_set1_epi8(static_cast<char>(x)); }

    // 字节加法（饱和无符号用 _mm256_adds_epu8；此处用环绕加法与标量行为一致）
    static auto add(reg a, reg b) -> reg { return _mm256_add_epi8(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm256_sub_epi8(a, b); }

    static auto min(reg a, reg b) -> reg { return _mm256_min_epu8(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm256_max_epu8(a, b); }

    // 水平求和：使用 _mm256_sad_epu8 对 32 个 uint8 求和
    // sad_epu8 将每 8 字节组的绝对差之和存入对应 64-bit lane 的低 16 位
    static auto reduce_sum(reg v) -> uint8_t
    {
        __m256i zero = _mm256_setzero_si256();
        __m256i sad  = _mm256_sad_epu8(v, zero);
        alignas(32) uint64_t buf[4];
        _mm256_store_si256(reinterpret_cast<__m256i*>(buf), sad);
        uint32_t total = static_cast<uint32_t>(buf[0] & 0xFFFF)
                       + static_cast<uint32_t>(buf[1] & 0xFFFF)
                       + static_cast<uint32_t>(buf[2] & 0xFFFF)
                       + static_cast<uint32_t>(buf[3] & 0xFFFF);
        return static_cast<uint8_t>(total);
    }
};

} // namespace bee::simd

#endif // BEE_SIMD_ENABLE_AVX2
