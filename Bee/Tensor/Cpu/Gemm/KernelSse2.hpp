/**
 * @File Cpu/Gemm/KernelSse2.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
 *
 * SSE2 微内核：128-bit 寄存器，FMA 用 mul+add 模拟，I32 mullo 用 SSE2
 * 能做到的 pmuludq + 重排（成本高，见下）。
 *
 * SSE2 没有：
 *   - FMA → 用 _mm_mul_ps/pd + _mm_add_ps/pd
 *   - _mm_mullo_epi32 （SSE4.1）→ I32 GEMM 退化为 4 标量乘 × SSE add_epi32 累加
 *   - _mm_cvtepi8_epi32（SSE4.1）→ I8→I32 用 unpack+srai 扩展
 */

#pragma once

#include <emmintrin.h> // SSE2

#include <cstdint>
#include <cstring>

namespace bee::cpu::gemm::sse2
{

// ─── SGEMM 4×4 微内核 ───────────────────────────────────────────────────────
// A_pack: 4 元素/列 × K；B_pack: 4 元素/行 × K
inline auto micro_kernel_sgemm_4x4(
    const float* __restrict A_pack,
    const float* __restrict B_pack,
    std::int64_t K,
    float* __restrict C,
    std::int64_t ldc
) -> void
{
    __m128 c0 = _mm_setzero_ps();
    __m128 c1 = _mm_setzero_ps();
    __m128 c2 = _mm_setzero_ps();
    __m128 c3 = _mm_setzero_ps();

    const float* pa = A_pack;
    const float* pb = B_pack;

    for (std::int64_t k = 0; k < K; ++k) {
        __m128 b   = _mm_loadu_ps(pb);
        __m128 a0  = _mm_load1_ps(pa + 0);
        __m128 a1  = _mm_load1_ps(pa + 1);
        __m128 a2  = _mm_load1_ps(pa + 2);
        __m128 a3  = _mm_load1_ps(pa + 3);
        c0         = _mm_add_ps(c0, _mm_mul_ps(a0, b));
        c1         = _mm_add_ps(c1, _mm_mul_ps(a1, b));
        c2         = _mm_add_ps(c2, _mm_mul_ps(a2, b));
        c3         = _mm_add_ps(c3, _mm_mul_ps(a3, b));
        pa        += 4;
        pb        += 4;
    }

    _mm_storeu_ps(C + 0 * ldc, _mm_add_ps(_mm_loadu_ps(C + 0 * ldc), c0));
    _mm_storeu_ps(C + 1 * ldc, _mm_add_ps(_mm_loadu_ps(C + 1 * ldc), c1));
    _mm_storeu_ps(C + 2 * ldc, _mm_add_ps(_mm_loadu_ps(C + 2 * ldc), c2));
    _mm_storeu_ps(C + 3 * ldc, _mm_add_ps(_mm_loadu_ps(C + 3 * ldc), c3));
}

// ─── DGEMM 4×2 微内核 ───────────────────────────────────────────────────────
inline auto micro_kernel_dgemm_4x2(
    const double* __restrict A_pack,
    const double* __restrict B_pack,
    std::int64_t K,
    double* __restrict C,
    std::int64_t ldc
) -> void
{
    __m128d c0 = _mm_setzero_pd();
    __m128d c1 = _mm_setzero_pd();
    __m128d c2 = _mm_setzero_pd();
    __m128d c3 = _mm_setzero_pd();

    const double* pa = A_pack;
    const double* pb = B_pack;

    for (std::int64_t k = 0; k < K; ++k) {
        __m128d b   = _mm_loadu_pd(pb);
        __m128d a0  = _mm_load1_pd(pa + 0);
        __m128d a1  = _mm_load1_pd(pa + 1);
        __m128d a2  = _mm_load1_pd(pa + 2);
        __m128d a3  = _mm_load1_pd(pa + 3);
        c0          = _mm_add_pd(c0, _mm_mul_pd(a0, b));
        c1          = _mm_add_pd(c1, _mm_mul_pd(a1, b));
        c2          = _mm_add_pd(c2, _mm_mul_pd(a2, b));
        c3          = _mm_add_pd(c3, _mm_mul_pd(a3, b));
        pa         += 4;
        pb         += 2;
    }

    _mm_storeu_pd(C + 0 * ldc, _mm_add_pd(_mm_loadu_pd(C + 0 * ldc), c0));
    _mm_storeu_pd(C + 1 * ldc, _mm_add_pd(_mm_loadu_pd(C + 1 * ldc), c1));
    _mm_storeu_pd(C + 2 * ldc, _mm_add_pd(_mm_loadu_pd(C + 2 * ldc), c2));
    _mm_storeu_pd(C + 3 * ldc, _mm_add_pd(_mm_loadu_pd(C + 3 * ldc), c3));
}

// ─── I32 4×4 微内核（SSE2 无 pmulld，用 4 次标量乘 + pack → add_epi32）─────
inline auto micro_kernel_i32_4x4(
    const std::int32_t* __restrict A_pack,
    const std::int32_t* __restrict B_pack,
    std::int64_t K,
    std::int32_t* __restrict C,
    std::int64_t ldc
) -> void
{
    alignas(16) std::int32_t c_tile[4][4] = {{0}};

    const std::int32_t* pa = A_pack;
    const std::int32_t* pb = B_pack;

    for (std::int64_t k = 0; k < K; ++k) {
        const std::int32_t b0 = pb[0];
        const std::int32_t b1 = pb[1];
        const std::int32_t b2 = pb[2];
        const std::int32_t b3 = pb[3];
        for (int r = 0; r < 4; ++r) {
            const std::int32_t a  = pa[r];
            c_tile[r][0]         += a * b0;
            c_tile[r][1]         += a * b1;
            c_tile[r][2]         += a * b2;
            c_tile[r][3]         += a * b3;
        }
        pa += 4;
        pb += 4;
    }

    for (int r = 0; r < 4; ++r) {
        __m128i c_old = _mm_loadu_si128(reinterpret_cast<const __m128i*>(C + r * ldc));
        __m128i c_new = _mm_load_si128(reinterpret_cast<const __m128i*>(&c_tile[r][0]));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(C + r * ldc), _mm_add_epi32(c_old, c_new));
    }
}

// ─── I8→I32 4×4 微内核（SSE2）─────────────────────────────────────────────
// A_pack: 4 int8 / k 列；B_pack: 4 int8 / k 行
inline auto micro_kernel_i8_i32_4x4(
    const std::int8_t* __restrict A_pack,
    const std::int8_t* __restrict B_pack,
    std::int64_t K,
    std::int32_t* __restrict C,
    std::int64_t ldc
) -> void
{
    alignas(16) std::int32_t c_tile[4][4] = {{0}};

    const std::int8_t* pa = A_pack;
    const std::int8_t* pb = B_pack;

    for (std::int64_t k = 0; k < K; ++k) {
        const std::int32_t b0 = static_cast<std::int32_t>(pb[0]);
        const std::int32_t b1 = static_cast<std::int32_t>(pb[1]);
        const std::int32_t b2 = static_cast<std::int32_t>(pb[2]);
        const std::int32_t b3 = static_cast<std::int32_t>(pb[3]);
        for (int r = 0; r < 4; ++r) {
            const std::int32_t a  = static_cast<std::int32_t>(pa[r]);
            c_tile[r][0]         += a * b0;
            c_tile[r][1]         += a * b1;
            c_tile[r][2]         += a * b2;
            c_tile[r][3]         += a * b3;
        }
        pa += 4;
        pb += 4;
    }

    for (int r = 0; r < 4; ++r) {
        __m128i c_old = _mm_loadu_si128(reinterpret_cast<const __m128i*>(C + r * ldc));
        __m128i c_new = _mm_load_si128(reinterpret_cast<const __m128i*>(&c_tile[r][0]));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(C + r * ldc), _mm_add_epi32(c_old, c_new));
    }
}

} // namespace bee::cpu::gemm::sse2
