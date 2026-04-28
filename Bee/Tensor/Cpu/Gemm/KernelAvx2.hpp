/**
 * @File Cpu/Gemm/KernelAvx2.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief CPU GEMM AVX2/FMA 微内核。
 *
 * AVX2/FMA 微内核：对 packed A/B panel 计算 MR×NR 的 C tile。
 * 所有 micro-kernel 写法均对齐：A_pack 按 MR 条带（每条 MR 个元素 × K 列），
 * B_pack 按 NR 条带（每条 NR 个元素 × K 行）。K 循环做 rank-1 外积累加。
 *
 * C 是行主序原始矩阵，micro-kernel 通过 ldc 写回。
 */

#pragma once

#include <immintrin.h>

#include <cstdint>

namespace bee::cpu::gemm::avx2
{

// ─── SGEMM 8×8 微内核（行主序 C）──────────────────────────────────────────────
// A_pack: 8 元素/列 × K → 布局 [k=0 a0..a7, k=1 a0..a7, ...]，连续 8*K floats
// B_pack: 8 元素/行 × K → 布局 [k=0 b0..b7, k=1 b0..b7, ...]，连续 K*8 floats
// 累加器 c[0..7]，每个 __m256 代表一行 × 8 列。
// 写回 C 时做 8 行 × 1 条 __m256 存储；ldc 是 C 的行距（= N）。

inline auto micro_kernel_sgemm_8x8(
    const float* __restrict A_pack,
    const float* __restrict B_pack,
    std::int64_t K,
    float* __restrict C,
    std::int64_t ldc
) -> void
{
    __m256 c0 = _mm256_setzero_ps();
    __m256 c1 = _mm256_setzero_ps();
    __m256 c2 = _mm256_setzero_ps();
    __m256 c3 = _mm256_setzero_ps();
    __m256 c4 = _mm256_setzero_ps();
    __m256 c5 = _mm256_setzero_ps();
    __m256 c6 = _mm256_setzero_ps();
    __m256 c7 = _mm256_setzero_ps();

    const float* pa = A_pack;
    const float* pb = B_pack;

    for (std::int64_t k = 0; k < K; ++k) {
        // 预取下一轮（64 字节 = 一个 cache line）
        _mm_prefetch(reinterpret_cast<const char*>(pa + 64), _MM_HINT_T0);
        _mm_prefetch(reinterpret_cast<const char*>(pb + 64), _MM_HINT_T0);

        __m256 b = _mm256_loadu_ps(pb);
        // 广播 A_pack[0..7] 分别到 8 个向量
        __m256 a0 = _mm256_broadcast_ss(pa + 0);
        __m256 a1 = _mm256_broadcast_ss(pa + 1);
        __m256 a2 = _mm256_broadcast_ss(pa + 2);
        __m256 a3 = _mm256_broadcast_ss(pa + 3);
        c0        = _mm256_fmadd_ps(a0, b, c0);
        c1        = _mm256_fmadd_ps(a1, b, c1);
        c2        = _mm256_fmadd_ps(a2, b, c2);
        c3        = _mm256_fmadd_ps(a3, b, c3);
        __m256 a4 = _mm256_broadcast_ss(pa + 4);
        __m256 a5 = _mm256_broadcast_ss(pa + 5);
        __m256 a6 = _mm256_broadcast_ss(pa + 6);
        __m256 a7 = _mm256_broadcast_ss(pa + 7);
        c4        = _mm256_fmadd_ps(a4, b, c4);
        c5        = _mm256_fmadd_ps(a5, b, c5);
        c6        = _mm256_fmadd_ps(a6, b, c6);
        c7        = _mm256_fmadd_ps(a7, b, c7);

        pa += 8;
        pb += 8;
    }

    _mm256_storeu_ps(C + 0 * ldc, _mm256_add_ps(_mm256_loadu_ps(C + 0 * ldc), c0));
    _mm256_storeu_ps(C + 1 * ldc, _mm256_add_ps(_mm256_loadu_ps(C + 1 * ldc), c1));
    _mm256_storeu_ps(C + 2 * ldc, _mm256_add_ps(_mm256_loadu_ps(C + 2 * ldc), c2));
    _mm256_storeu_ps(C + 3 * ldc, _mm256_add_ps(_mm256_loadu_ps(C + 3 * ldc), c3));
    _mm256_storeu_ps(C + 4 * ldc, _mm256_add_ps(_mm256_loadu_ps(C + 4 * ldc), c4));
    _mm256_storeu_ps(C + 5 * ldc, _mm256_add_ps(_mm256_loadu_ps(C + 5 * ldc), c5));
    _mm256_storeu_ps(C + 6 * ldc, _mm256_add_ps(_mm256_loadu_ps(C + 6 * ldc), c6));
    _mm256_storeu_ps(C + 7 * ldc, _mm256_add_ps(_mm256_loadu_ps(C + 7 * ldc), c7));
}

// ─── DGEMM 8×4 微内核 ───────────────────────────────────────────────────────
// 参考 AVX512-DGEMM 教程 k7：两条 __m256d（每行一对 2×4d），共 8 行。
// A_pack: 8 元素/列 × K → [k=0 a0..a7, ...]
// B_pack: 4 元素/行 × K → [k=0 b0..b3, ...]

inline auto micro_kernel_dgemm_8x4(
    const double* __restrict A_pack,
    const double* __restrict B_pack,
    std::int64_t K,
    double* __restrict C,
    std::int64_t ldc
) -> void
{
    // c[i] = i 行的 4 列（一条 __m256d）
    __m256d c0 = _mm256_setzero_pd();
    __m256d c1 = _mm256_setzero_pd();
    __m256d c2 = _mm256_setzero_pd();
    __m256d c3 = _mm256_setzero_pd();
    __m256d c4 = _mm256_setzero_pd();
    __m256d c5 = _mm256_setzero_pd();
    __m256d c6 = _mm256_setzero_pd();
    __m256d c7 = _mm256_setzero_pd();

    const double* pa = A_pack;
    const double* pb = B_pack;

    for (std::int64_t k = 0; k < K; ++k) {
        _mm_prefetch(reinterpret_cast<const char*>(pa + 64), _MM_HINT_T0);
        _mm_prefetch(reinterpret_cast<const char*>(pb + 64), _MM_HINT_T0);

        __m256d b  = _mm256_loadu_pd(pb);
        __m256d a0 = _mm256_broadcast_sd(pa + 0);
        __m256d a1 = _mm256_broadcast_sd(pa + 1);
        __m256d a2 = _mm256_broadcast_sd(pa + 2);
        __m256d a3 = _mm256_broadcast_sd(pa + 3);
        c0         = _mm256_fmadd_pd(a0, b, c0);
        c1         = _mm256_fmadd_pd(a1, b, c1);
        c2         = _mm256_fmadd_pd(a2, b, c2);
        c3         = _mm256_fmadd_pd(a3, b, c3);
        __m256d a4 = _mm256_broadcast_sd(pa + 4);
        __m256d a5 = _mm256_broadcast_sd(pa + 5);
        __m256d a6 = _mm256_broadcast_sd(pa + 6);
        __m256d a7 = _mm256_broadcast_sd(pa + 7);
        c4         = _mm256_fmadd_pd(a4, b, c4);
        c5         = _mm256_fmadd_pd(a5, b, c5);
        c6         = _mm256_fmadd_pd(a6, b, c6);
        c7         = _mm256_fmadd_pd(a7, b, c7);

        pa += 8;
        pb += 4;
    }

    _mm256_storeu_pd(C + 0 * ldc, _mm256_add_pd(_mm256_loadu_pd(C + 0 * ldc), c0));
    _mm256_storeu_pd(C + 1 * ldc, _mm256_add_pd(_mm256_loadu_pd(C + 1 * ldc), c1));
    _mm256_storeu_pd(C + 2 * ldc, _mm256_add_pd(_mm256_loadu_pd(C + 2 * ldc), c2));
    _mm256_storeu_pd(C + 3 * ldc, _mm256_add_pd(_mm256_loadu_pd(C + 3 * ldc), c3));
    _mm256_storeu_pd(C + 4 * ldc, _mm256_add_pd(_mm256_loadu_pd(C + 4 * ldc), c4));
    _mm256_storeu_pd(C + 5 * ldc, _mm256_add_pd(_mm256_loadu_pd(C + 5 * ldc), c5));
    _mm256_storeu_pd(C + 6 * ldc, _mm256_add_pd(_mm256_loadu_pd(C + 6 * ldc), c6));
    _mm256_storeu_pd(C + 7 * ldc, _mm256_add_pd(_mm256_loadu_pd(C + 7 * ldc), c7));
}

// ─── I32GEMM 8×8 微内核（AVX2 has vpmulld + vpaddd）──────────────────────────
inline auto micro_kernel_i32_8x8(
    const std::int32_t* __restrict A_pack,
    const std::int32_t* __restrict B_pack,
    std::int64_t K,
    std::int32_t* __restrict C,
    std::int64_t ldc
) -> void
{
    __m256i c0 = _mm256_setzero_si256();
    __m256i c1 = _mm256_setzero_si256();
    __m256i c2 = _mm256_setzero_si256();
    __m256i c3 = _mm256_setzero_si256();
    __m256i c4 = _mm256_setzero_si256();
    __m256i c5 = _mm256_setzero_si256();
    __m256i c6 = _mm256_setzero_si256();
    __m256i c7 = _mm256_setzero_si256();

    const std::int32_t* pa = A_pack;
    const std::int32_t* pb = B_pack;

    for (std::int64_t k = 0; k < K; ++k) {
        _mm_prefetch(reinterpret_cast<const char*>(pa + 64), _MM_HINT_T0);
        _mm_prefetch(reinterpret_cast<const char*>(pb + 64), _MM_HINT_T0);

        __m256i b  = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pb));
        __m256i a0 = _mm256_set1_epi32(pa[0]);
        __m256i a1 = _mm256_set1_epi32(pa[1]);
        __m256i a2 = _mm256_set1_epi32(pa[2]);
        __m256i a3 = _mm256_set1_epi32(pa[3]);
        c0         = _mm256_add_epi32(c0, _mm256_mullo_epi32(a0, b));
        c1         = _mm256_add_epi32(c1, _mm256_mullo_epi32(a1, b));
        c2         = _mm256_add_epi32(c2, _mm256_mullo_epi32(a2, b));
        c3         = _mm256_add_epi32(c3, _mm256_mullo_epi32(a3, b));
        __m256i a4 = _mm256_set1_epi32(pa[4]);
        __m256i a5 = _mm256_set1_epi32(pa[5]);
        __m256i a6 = _mm256_set1_epi32(pa[6]);
        __m256i a7 = _mm256_set1_epi32(pa[7]);
        c4         = _mm256_add_epi32(c4, _mm256_mullo_epi32(a4, b));
        c5         = _mm256_add_epi32(c5, _mm256_mullo_epi32(a5, b));
        c6         = _mm256_add_epi32(c6, _mm256_mullo_epi32(a6, b));
        c7         = _mm256_add_epi32(c7, _mm256_mullo_epi32(a7, b));

        pa += 8;
        pb += 8;
    }

#define ST_I32(row, vec)                                                                                                                          \
    _mm256_storeu_si256(                                                                                                                          \
        reinterpret_cast<__m256i*>(C + (row) * ldc), _mm256_add_epi32(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(C + (row) * ldc)), vec) \
    )
    ST_I32(0, c0);
    ST_I32(1, c1);
    ST_I32(2, c2);
    ST_I32(3, c3);
    ST_I32(4, c4);
    ST_I32(5, c5);
    ST_I32(6, c6);
    ST_I32(7, c7);
#undef ST_I32
}

// ─── I8→I32 8×8 微内核（AVX2）────────────────────────────────────────────────
// 为简化与保正确性：用 _mm256_cvtepi8_epi16（符号扩展）把 A 的广播元素与 B 的 8 元素
// 提升到 int16，相乘得到 int16，再符号扩展到 int32 累加。
// 注：性能较 vpmaddubsw/vpmaddwd 级联低，但实现简单可靠，便于保持 I8×I8→I32
// 路径的数值正确性。

inline auto micro_kernel_i8_i32_8x8(
    const std::int8_t* __restrict A_pack,
    const std::int8_t* __restrict B_pack,
    std::int64_t K,
    std::int32_t* __restrict C,
    std::int64_t ldc
) -> void
{
    __m256i c0 = _mm256_setzero_si256();
    __m256i c1 = _mm256_setzero_si256();
    __m256i c2 = _mm256_setzero_si256();
    __m256i c3 = _mm256_setzero_si256();
    __m256i c4 = _mm256_setzero_si256();
    __m256i c5 = _mm256_setzero_si256();
    __m256i c6 = _mm256_setzero_si256();
    __m256i c7 = _mm256_setzero_si256();

    const std::int8_t* pa = A_pack;
    const std::int8_t* pb = B_pack;

    for (std::int64_t k = 0; k < K; ++k) {
        // 读 8 个 int8 B 元素 → 扩展到 __m256i(8×int32)
        __m128i b8  = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pb));
        __m256i b32 = _mm256_cvtepi8_epi32(b8);

#define FMA_I8(row)                                                         \
    do {                                                                    \
        __m256i av = _mm256_set1_epi32(static_cast<std::int32_t>(pa[row])); \
        c##row     = _mm256_add_epi32(c##row, _mm256_mullo_epi32(av, b32)); \
    } while (0)
        FMA_I8(0);
        FMA_I8(1);
        FMA_I8(2);
        FMA_I8(3);
        FMA_I8(4);
        FMA_I8(5);
        FMA_I8(6);
        FMA_I8(7);
#undef FMA_I8

        pa += 8;
        pb += 8;
    }

#define ST_I32(row, vec)                                                                                                                          \
    _mm256_storeu_si256(                                                                                                                          \
        reinterpret_cast<__m256i*>(C + (row) * ldc), _mm256_add_epi32(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(C + (row) * ldc)), vec) \
    )
    ST_I32(0, c0);
    ST_I32(1, c1);
    ST_I32(2, c2);
    ST_I32(3, c3);
    ST_I32(4, c4);
    ST_I32(5, c5);
    ST_I32(6, c6);
    ST_I32(7, c7);
#undef ST_I32
}

} // namespace bee::cpu::gemm::avx2
