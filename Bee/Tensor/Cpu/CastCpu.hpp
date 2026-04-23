#pragma once

// CPU 类型转换内核：基于 ISA 标签的模板化实现
// - 常用数值对（F32↔F64/I32/U8）在 AVX2/SSE2 下走 intrinsics
// - 其余组合回落到 static_cast 标量循环
// - 大 n 走 parallel_for 切块

#include "Tensor/Core/DType.hpp"
#include "SIMD/SIMD.hpp"
#include "Base/Parallel/ParallelFor.hpp"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <algorithm>

#if defined(BEE_SIMD_ENABLE_SSE2)
    #include <emmintrin.h>
    #include <smmintrin.h>
#endif
#if defined(BEE_SIMD_ENABLE_AVX2)
    #include <immintrin.h>
#endif
#if defined(BEE_SIMD_ENABLE_AVX512)
    #include <immintrin.h>
#endif

namespace bee::cpu
{

// ─── 标量兜底 ────────────────────────────────────────────────────────────────
template <typename Src, typename Dst>
inline void cast_scalar_chunk(const Src* s, Dst* d, std::int64_t n)
{
    for (std::int64_t i = 0; i < n; ++i) {
        if constexpr (std::is_same_v<Dst, bool>) {
            d[i] = (s[i] != Src{0});
        } else if constexpr (std::is_same_v<Src, bool>) {
            d[i] = static_cast<Dst>(s[i] ? Src{1} : Src{0});
        } else {
            d[i] = static_cast<Dst>(s[i]);
        }
    }
}

// ─── 模板默认：标量 ──────────────────────────────────────────────────────────
template <typename Src, typename Dst, typename ISA>
inline void cast_simd_chunk(const Src* s, Dst* d, std::int64_t n)
{
    cast_scalar_chunk<Src, Dst>(s, d, n);
}

// ─── SSE2 / SSE4.1 特化 ──────────────────────────────────────────────────────
#if defined(BEE_SIMD_ENABLE_SSE2)

// F32 → F64：_mm_cvtps_pd（低 2 × f32 → 2 × f64）
template <>
inline void cast_simd_chunk<float, double, simd::IsaSse2>(const float* s, double* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 4 <= n; i += 4) {
        __m128 v = _mm_loadu_ps(s + i);
        _mm_storeu_pd(d + i, _mm_cvtps_pd(v));
        _mm_storeu_pd(d + i + 2, _mm_cvtps_pd(_mm_movehl_ps(v, v)));
    }
    for (; i < n; ++i) d[i] = static_cast<double>(s[i]);
}

// F64 → F32
template <>
inline void cast_simd_chunk<double, float, simd::IsaSse2>(const double* s, float* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 4 <= n; i += 4) {
        __m128 lo = _mm_cvtpd_ps(_mm_loadu_pd(s + i));
        __m128 hi = _mm_cvtpd_ps(_mm_loadu_pd(s + i + 2));
        _mm_storeu_ps(d + i, _mm_movelh_ps(lo, hi));
    }
    for (; i < n; ++i) d[i] = static_cast<float>(s[i]);
}

// F32 → I32 (truncate)
template <>
inline void cast_simd_chunk<float, std::int32_t, simd::IsaSse2>(const float* s, std::int32_t* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 4 <= n; i += 4) {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + i), _mm_cvttps_epi32(_mm_loadu_ps(s + i)));
    }
    for (; i < n; ++i) d[i] = static_cast<std::int32_t>(s[i]);
}

// I32 → F32
template <>
inline void cast_simd_chunk<std::int32_t, float, simd::IsaSse2>(const std::int32_t* s, float* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 4 <= n; i += 4) {
        _mm_storeu_ps(d + i, _mm_cvtepi32_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(s + i))));
    }
    for (; i < n; ++i) d[i] = static_cast<float>(s[i]);
}

// U8 → F32：SSE4.1 _mm_cvtepu8_epi32
template <>
inline void cast_simd_chunk<std::uint8_t, float, simd::IsaSse2>(const std::uint8_t* s, float* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 4 <= n; i += 4) {
        __m128i u = _mm_cvtsi32_si128(*reinterpret_cast<const std::int32_t*>(s + i));
        __m128i w = _mm_cvtepu8_epi32(u);
        _mm_storeu_ps(d + i, _mm_cvtepi32_ps(w));
    }
    for (; i < n; ++i) d[i] = static_cast<float>(s[i]);
}

// F32 → U8：cvttps_epi32 → packs_epi32 → packus_epi16（带饱和裁剪到 [0,255]）
template <>
inline void cast_simd_chunk<float, std::uint8_t, simd::IsaSse2>(const float* s, std::uint8_t* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 16 <= n; i += 16) {
        __m128i i0 = _mm_cvttps_epi32(_mm_loadu_ps(s + i));
        __m128i i1 = _mm_cvttps_epi32(_mm_loadu_ps(s + i + 4));
        __m128i i2 = _mm_cvttps_epi32(_mm_loadu_ps(s + i + 8));
        __m128i i3 = _mm_cvttps_epi32(_mm_loadu_ps(s + i + 12));
        __m128i p01 = _mm_packus_epi32(i0, i1);
        __m128i p23 = _mm_packus_epi32(i2, i3);
        __m128i p   = _mm_packus_epi16(p01, p23);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + i), p);
    }
    for (; i < n; ++i) {
        float v = s[i];
        if (v < 0.0f) v = 0.0f;
        if (v > 255.0f) v = 255.0f;
        d[i] = static_cast<std::uint8_t>(v);
    }
}

// F32 → Bool：比较零后把 0/0xFFFFFFFF 折叠成 0/1
template <>
inline void cast_simd_chunk<float, bool, simd::IsaSse2>(const float* s, bool* d, std::int64_t n)
{
    std::int64_t i = 0;
    const __m128 zero = _mm_setzero_ps();
    for (; i + 4 <= n; i += 4) {
        __m128 v    = _mm_loadu_ps(s + i);
        __m128 mask = _mm_cmpneq_ps(v, zero);
        __m128i bi  = _mm_and_si128(_mm_castps_si128(mask), _mm_set1_epi32(1));
        // 四个 32-bit → 四个 byte：shuffle 低字节到前 4
        __m128i packed = _mm_shuffle_epi8(bi, _mm_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1));
        std::int32_t v4 = _mm_cvtsi128_si32(packed);
        std::memcpy(d + i, &v4, 4);
    }
    for (; i < n; ++i) d[i] = (s[i] != 0.0f);
}

#endif // BEE_SIMD_ENABLE_SSE2

// ─── AVX2 特化 ───────────────────────────────────────────────────────────────
#if defined(BEE_SIMD_ENABLE_AVX2)

template <>
inline void cast_simd_chunk<float, double, simd::IsaAvx2>(const float* s, double* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 v = _mm256_loadu_ps(s + i);
        _mm256_storeu_pd(d + i,     _mm256_cvtps_pd(_mm256_castps256_ps128(v)));
        _mm256_storeu_pd(d + i + 4, _mm256_cvtps_pd(_mm256_extractf128_ps(v, 1)));
    }
    for (; i < n; ++i) d[i] = static_cast<double>(s[i]);
}

template <>
inline void cast_simd_chunk<double, float, simd::IsaAvx2>(const double* s, float* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m128 lo = _mm256_cvtpd_ps(_mm256_loadu_pd(s + i));
        __m128 hi = _mm256_cvtpd_ps(_mm256_loadu_pd(s + i + 4));
        _mm256_storeu_ps(d + i, _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1));
    }
    for (; i < n; ++i) d[i] = static_cast<float>(s[i]);
}

template <>
inline void cast_simd_chunk<float, std::int32_t, simd::IsaAvx2>(const float* s, std::int32_t* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 8 <= n; i += 8) {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(d + i), _mm256_cvttps_epi32(_mm256_loadu_ps(s + i)));
    }
    for (; i < n; ++i) d[i] = static_cast<std::int32_t>(s[i]);
}

template <>
inline void cast_simd_chunk<std::int32_t, float, simd::IsaAvx2>(const std::int32_t* s, float* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 8 <= n; i += 8) {
        _mm256_storeu_ps(d + i, _mm256_cvtepi32_ps(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + i))));
    }
    for (; i < n; ++i) d[i] = static_cast<float>(s[i]);
}

template <>
inline void cast_simd_chunk<std::uint8_t, float, simd::IsaAvx2>(const std::uint8_t* s, float* d, std::int64_t n)
{
    std::int64_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m128i u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(s + i));
        __m256i w = _mm256_cvtepu8_epi32(u);
        _mm256_storeu_ps(d + i, _mm256_cvtepi32_ps(w));
    }
    for (; i < n; ++i) d[i] = static_cast<float>(s[i]);
}

template <>
inline void cast_simd_chunk<float, std::uint8_t, simd::IsaAvx2>(const float* s, std::uint8_t* d, std::int64_t n)
{
    std::int64_t i = 0;
    // 一轮 16 元素：两次 256 → cvttps_epi32 → pack
    for (; i + 16 <= n; i += 16) {
        __m256i i0 = _mm256_cvttps_epi32(_mm256_loadu_ps(s + i));
        __m256i i1 = _mm256_cvttps_epi32(_mm256_loadu_ps(s + i + 8));
        // pack 32→16 with unsigned saturation（跨 lane），随后 16→8
        __m256i p16 = _mm256_packus_epi32(i0, i1);
        // _mm256_packus_epi32 的输出 lane 顺序是 [a0 a1 b0 b1]（每 lane 独立），需 permute4x64 重排
        p16 = _mm256_permute4x64_epi64(p16, 0b11'01'10'00);
        __m256i p8 = _mm256_packus_epi16(p16, p16);
        p8 = _mm256_permute4x64_epi64(p8, 0b11'01'10'00);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + i), _mm256_castsi256_si128(p8));
    }
    for (; i < n; ++i) {
        float v = s[i];
        if (v < 0.0f) v = 0.0f;
        if (v > 255.0f) v = 255.0f;
        d[i] = static_cast<std::uint8_t>(v);
    }
}

template <>
inline void cast_simd_chunk<float, bool, simd::IsaAvx2>(const float* s, bool* d, std::int64_t n)
{
    std::int64_t i = 0;
    const __m256 zero = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 v = _mm256_loadu_ps(s + i);
        __m256 mask = _mm256_cmp_ps(v, zero, _CMP_NEQ_UQ);
        __m256i bi = _mm256_and_si256(_mm256_castps_si256(mask), _mm256_set1_epi32(1));
        // 8 × 32-bit → 8 × byte：拆两个 128 lane 后 packus
        __m128i lo = _mm256_castsi256_si128(bi);
        __m128i hi = _mm256_extracti128_si256(bi, 1);
        __m128i p16 = _mm_packus_epi32(lo, hi);
        __m128i p8  = _mm_packus_epi16(p16, p16);
        std::int64_t v8 = _mm_cvtsi128_si64(p8);
        std::memcpy(d + i, &v8, 8);
    }
    for (; i < n; ++i) d[i] = (s[i] != 0.0f);
}

#endif // BEE_SIMD_ENABLE_AVX2

// ─── 并行化包装 ──────────────────────────────────────────────────────────────
inline constexpr std::int64_t kCastParallelElems = 64 * 1024;
inline constexpr std::int64_t kCastGrainBytes    = 128 * 1024;

template <typename Src, typename Dst, typename ISA>
inline void cpu_cast_linear_parallel(const Src* s, Dst* d, std::int64_t n)
{
    const std::int64_t elem_bytes = static_cast<std::int64_t>(sizeof(Src) + sizeof(Dst));
    const std::int64_t grain = std::max<std::int64_t>(1, kCastGrainBytes / std::max<std::int64_t>(1, elem_bytes));
    if (n < kCastParallelElems) {
        cast_simd_chunk<Src, Dst, ISA>(s, d, n);
        return;
    }
    parallel::parallel_for(
        std::size_t{0}, static_cast<std::size_t>(n), static_cast<std::size_t>(grain),
        [&](std::size_t lo, std::size_t hi) {
            cast_simd_chunk<Src, Dst, ISA>(s + lo, d + lo, static_cast<std::int64_t>(hi - lo));
        });
}

// ─── 顶层分派：根据运行期 (src_dt, dst_dt) 选择内核 ──────────────────────────
template <typename ISA>
inline void cpu_cast_dispatch(DType src_dt, DType dst_dt, const void* src, void* dst, std::int64_t n)
{
#define BEE_CAST_CASE(SDT, DDT, STYPE, DTYPE)                                                     \
    if (src_dt == DType::SDT && dst_dt == DType::DDT) {                                           \
        cpu_cast_linear_parallel<STYPE, DTYPE, ISA>(                                              \
            static_cast<const STYPE*>(src), static_cast<DTYPE*>(dst), n);                         \
        return;                                                                                   \
    }

    // 先尝试所有 (src, dst) 组合：相同 dtype 不在此路径（顶层已处理）
    BEE_CAST_CASE(Bool, U8,   bool, std::uint8_t)
    BEE_CAST_CASE(Bool, I32,  bool, std::int32_t)
    BEE_CAST_CASE(Bool, I64,  bool, std::int64_t)
    BEE_CAST_CASE(Bool, F32,  bool, float)
    BEE_CAST_CASE(Bool, F64,  bool, double)

    BEE_CAST_CASE(U8,   Bool, std::uint8_t, bool)
    BEE_CAST_CASE(U8,   I32,  std::uint8_t, std::int32_t)
    BEE_CAST_CASE(U8,   I64,  std::uint8_t, std::int64_t)
    BEE_CAST_CASE(U8,   F32,  std::uint8_t, float)
    BEE_CAST_CASE(U8,   F64,  std::uint8_t, double)

    BEE_CAST_CASE(I32,  Bool, std::int32_t, bool)
    BEE_CAST_CASE(I32,  U8,   std::int32_t, std::uint8_t)
    BEE_CAST_CASE(I32,  I64,  std::int32_t, std::int64_t)
    BEE_CAST_CASE(I32,  F32,  std::int32_t, float)
    BEE_CAST_CASE(I32,  F64,  std::int32_t, double)

    BEE_CAST_CASE(I64,  Bool, std::int64_t, bool)
    BEE_CAST_CASE(I64,  U8,   std::int64_t, std::uint8_t)
    BEE_CAST_CASE(I64,  I32,  std::int64_t, std::int32_t)
    BEE_CAST_CASE(I64,  F32,  std::int64_t, float)
    BEE_CAST_CASE(I64,  F64,  std::int64_t, double)

    BEE_CAST_CASE(F32,  Bool, float, bool)
    BEE_CAST_CASE(F32,  U8,   float, std::uint8_t)
    BEE_CAST_CASE(F32,  I32,  float, std::int32_t)
    BEE_CAST_CASE(F32,  I64,  float, std::int64_t)
    BEE_CAST_CASE(F32,  F64,  float, double)

    BEE_CAST_CASE(F64,  Bool, double, bool)
    BEE_CAST_CASE(F64,  U8,   double, std::uint8_t)
    BEE_CAST_CASE(F64,  I32,  double, std::int32_t)
    BEE_CAST_CASE(F64,  I64,  double, std::int64_t)
    BEE_CAST_CASE(F64,  F32,  double, float)

#undef BEE_CAST_CASE
    // 未覆盖组合（如 F16/BF16 占位类型）：静默返回
}

} // namespace bee::cpu
