/**
 * @File Stream.hpp
 * @Brief 非时序流式写入原语。
 *
 * 设计动机：当输出缓冲区明显超过末级缓存容量时，使用非时序写入可以
 * 避免读所有权事务和缓存污染，适合 ElementWise、Copy 等主要受内存带宽
 * 限制的算子。该优化只改变写入路径，不改变内存内容或异常语义。
 *
 * 使用规范：
 *   1. 只在指针按寄存器宽度对齐时调用对应特化；未满足对齐要求时应使用
 *      storeu 或默认回退实现。
 *   2. 一个连续批次的非时序写入结束后需调用 simd::sfence() 一次，确保
 *      后续读取或跨线程观察前写入已经提交。
 */

#pragma once

#include "Traits.hpp"

#if defined(BEE_SIMD_ENABLE_SSE2) || defined(BEE_SIMD_ENABLE_AVX2) || defined(BEE_SIMD_ENABLE_AVX512)
    #include <immintrin.h>
#endif

#include <cstdint>

namespace bee::simd
{

// 默认回退：对无非时序写入特化的 (T, ISA) 组合，使用普通非对齐写入。
// 该路径优先保证正确性，性能特征与对应后端的 storeu 相同。
template <typename T, typename ISA>
inline void simd_stream(T* p, typename SimdBackend<T, ISA>::reg v) noexcept
{
    SimdBackend<T, ISA>::storeu(p, v);
}

// ── SSE2 ─────────────────────────────────────────────────────────────────────
#ifdef BEE_SIMD_ENABLE_SSE2
template <>
inline void simd_stream<float, IsaSse2>(float* p, __m128 v) noexcept
{
    _mm_stream_ps(p, v);
}
template <>
inline void simd_stream<double, IsaSse2>(double* p, __m128d v) noexcept
{
    _mm_stream_pd(p, v);
}
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
inline void simd_stream<float, IsaAvx2>(float* p, __m256 v) noexcept
{
    _mm256_stream_ps(p, v);
}
template <>
inline void simd_stream<double, IsaAvx2>(double* p, __m256d v) noexcept
{
    _mm256_stream_pd(p, v);
}
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
inline void simd_stream<float, IsaAvx512>(float* p, __m512 v) noexcept
{
    _mm512_stream_ps(p, v);
}
template <>
inline void simd_stream<double, IsaAvx512>(double* p, __m512d v) noexcept
{
    _mm512_stream_pd(p, v);
}
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
