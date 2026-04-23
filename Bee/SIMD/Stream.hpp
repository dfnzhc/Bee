/**
 * @File Stream.hpp
 * @Brief Non-temporal (streaming) store 原语。
 *
 * 设计动机：当输出缓冲区明显超过 LLC（通常 ≥ L2 的若干倍）时，用 NT-store
 * 跳过读 for-ownership 与缓存占用，是 ElementWise/Copy 等带宽敏感算子的
 * 标准加速手段。NT-store 要求指针按 SIMD 寄存器宽度对齐。
 *
 * 使用规范：
 *   1. 只在 (p % register_size == 0) 时调用；否则退化到 storeu。
 *   2. 一个批次的 NT-store 结束后需调用 simd::sfence() 一次。
 */

#pragma once

#include "Traits.hpp"

#if defined(BEE_SIMD_ENABLE_SSE2) || defined(BEE_SIMD_ENABLE_AVX2) || defined(BEE_SIMD_ENABLE_AVX512)
    #include <immintrin.h>
#endif

#include <cstdint>

namespace bee::simd
{

// 默认回退：对无 NT 支持的 (T, ISA) 组合，使用普通 storeu 行为。
template <typename T, typename ISA>
inline void simd_stream(T* p, typename SimdBackend<T, ISA>::reg v) noexcept
{
    SimdBackend<T, ISA>::storeu(p, v);
}

// ── SSE2 ─────────────────────────────────────────────────────────────────────
#ifdef BEE_SIMD_ENABLE_SSE2
template <>
inline void simd_stream<float, IsaSse2>(float* p, __m128 v) noexcept { _mm_stream_ps(p, v); }
template <>
inline void simd_stream<double, IsaSse2>(double* p, __m128d v) noexcept { _mm_stream_pd(p, v); }
template <>
inline void simd_stream<int32_t, IsaSse2>(int32_t* p, __m128i v) noexcept
{
    _mm_stream_si128(reinterpret_cast<__m128i*>(p), v);
}
template <>
inline void simd_stream<int64_t, IsaSse2>(int64_t* p, __m128i v) noexcept
{
    _mm_stream_si128(reinterpret_cast<__m128i*>(p), v);
}
template <>
inline void simd_stream<uint8_t, IsaSse2>(uint8_t* p, __m128i v) noexcept
{
    _mm_stream_si128(reinterpret_cast<__m128i*>(p), v);
}
#endif

// ── AVX2 ─────────────────────────────────────────────────────────────────────
#ifdef BEE_SIMD_ENABLE_AVX2
template <>
inline void simd_stream<float, IsaAvx2>(float* p, __m256 v) noexcept { _mm256_stream_ps(p, v); }
template <>
inline void simd_stream<double, IsaAvx2>(double* p, __m256d v) noexcept { _mm256_stream_pd(p, v); }
template <>
inline void simd_stream<int32_t, IsaAvx2>(int32_t* p, __m256i v) noexcept
{
    _mm256_stream_si256(reinterpret_cast<__m256i*>(p), v);
}
template <>
inline void simd_stream<int64_t, IsaAvx2>(int64_t* p, __m256i v) noexcept
{
    _mm256_stream_si256(reinterpret_cast<__m256i*>(p), v);
}
template <>
inline void simd_stream<uint8_t, IsaAvx2>(uint8_t* p, __m256i v) noexcept
{
    _mm256_stream_si256(reinterpret_cast<__m256i*>(p), v);
}
#endif

// ── AVX512 ───────────────────────────────────────────────────────────────────
#ifdef BEE_SIMD_ENABLE_AVX512
template <>
inline void simd_stream<float, IsaAvx512>(float* p, __m512 v) noexcept { _mm512_stream_ps(p, v); }
template <>
inline void simd_stream<double, IsaAvx512>(double* p, __m512d v) noexcept { _mm512_stream_pd(p, v); }
template <>
inline void simd_stream<int32_t, IsaAvx512>(int32_t* p, __m512i v) noexcept
{
    _mm512_stream_si512(reinterpret_cast<__m512i*>(p), v);
}
template <>
inline void simd_stream<int64_t, IsaAvx512>(int64_t* p, __m512i v) noexcept
{
    _mm512_stream_si512(reinterpret_cast<__m512i*>(p), v);
}
    #ifdef __AVX512BW__
template <>
inline void simd_stream<uint8_t, IsaAvx512>(uint8_t* p, __m512i v) noexcept
{
    _mm512_stream_si512(reinterpret_cast<__m512i*>(p), v);
}
    #endif
#endif

} // namespace bee::simd
