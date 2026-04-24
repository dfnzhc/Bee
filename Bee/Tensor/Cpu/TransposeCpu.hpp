#pragma once

// CPU 2D strided→contiguous 拷贝（即 transpose 物化）
// - 按 TILE × TILE 分块，显著改善 L1 局部性
// - F32 在 AVX2 下用 8×8 寄存器转置 + 块内 NT-store（当 bytes 足够大）
// - 其他 elem_sz 走通用标量块拷贝
// - 外层 tile-row 维度走 parallel_for

#include "SIMD/SIMD.hpp"
#include "Base/Parallel/ParallelFor.hpp"

#include <cstdint>
#include <cstring>
#include <cstddef>
#include <algorithm>

#if defined(BEE_SIMD_ENABLE_AVX2)
    #include <immintrin.h>
#endif

namespace bee::cpu
{

inline constexpr std::int64_t kTrTile          = 32; // 字节通用分块
inline constexpr std::int64_t kTrParallelRows  = 64; // 行数小于该阈值单线程
inline constexpr std::int64_t kTrGrainTileRows = 2;  // 每任务多少 tile-row

// ── 通用：按字节拷贝 tile（任意 elem_sz）────────────────────────────────────
// src 按 (rows, cols) 逻辑，src[r,c] 的字节偏移 = r*src_row_bytes + c*src_col_bytes
// dst 按连续行优先 [rows, cols] 写入
inline void transpose_2d_generic(
    const std::uint8_t* src_base,
    std::int64_t        src_row_bytes, // src 中"行步进"字节（对应输出列方向步进）
    std::int64_t        src_col_bytes, // src 中"列步进"字节（对应输出行方向步进）
    std::uint8_t*       dst,
    std::int64_t        rows, // 输出行数
    std::int64_t        cols, // 输出列数
    std::size_t         elem_sz
)
{
    const std::int64_t dst_row_bytes = cols * static_cast<std::int64_t>(elem_sz);
    const std::int64_t tile          = kTrTile;

    auto run_tile_row = [&](std::int64_t r0, std::int64_t r1) {
        for (std::int64_t c0 = 0; c0 < cols; c0 += tile) {
            const std::int64_t c1 = std::min(c0 + tile, cols);
            for (std::int64_t r = r0; r < r1; ++r) {
                const std::uint8_t* src_row_ptr_base = src_base + r * src_row_bytes;
                std::uint8_t*       dst_row_ptr      = dst + r * dst_row_bytes;
                for (std::int64_t c = c0; c < c1; ++c) {
                    std::memcpy(dst_row_ptr + c * static_cast<std::int64_t>(elem_sz), src_row_ptr_base + c * src_col_bytes, elem_sz);
                }
            }
        }
    };

    if (rows < kTrParallelRows) {
        run_tile_row(0, rows);
        return;
    }

    const std::size_t row_grain = static_cast<std::size_t>(tile) * static_cast<std::size_t>(kTrGrainTileRows);
    parallel::parallel_for(std::size_t{0}, static_cast<std::size_t>(rows), row_grain, [&](std::size_t lo, std::size_t hi) {
        run_tile_row(static_cast<std::int64_t>(lo), static_cast<std::int64_t>(hi));
    });
}

// ── F32 AVX2 特化：8×8 寄存器转置 ────────────────────────────────────────────
#if defined(BEE_SIMD_ENABLE_AVX2)

inline void transpose_8x8_ps_avx2(const float* src, std::int64_t src_stride, float* dst, std::int64_t dst_stride)
{
    __m256 r0 = _mm256_loadu_ps(src + 0 * src_stride);
    __m256 r1 = _mm256_loadu_ps(src + 1 * src_stride);
    __m256 r2 = _mm256_loadu_ps(src + 2 * src_stride);
    __m256 r3 = _mm256_loadu_ps(src + 3 * src_stride);
    __m256 r4 = _mm256_loadu_ps(src + 4 * src_stride);
    __m256 r5 = _mm256_loadu_ps(src + 5 * src_stride);
    __m256 r6 = _mm256_loadu_ps(src + 6 * src_stride);
    __m256 r7 = _mm256_loadu_ps(src + 7 * src_stride);

    __m256 t0 = _mm256_unpacklo_ps(r0, r1);
    __m256 t1 = _mm256_unpackhi_ps(r0, r1);
    __m256 t2 = _mm256_unpacklo_ps(r2, r3);
    __m256 t3 = _mm256_unpackhi_ps(r2, r3);
    __m256 t4 = _mm256_unpacklo_ps(r4, r5);
    __m256 t5 = _mm256_unpackhi_ps(r4, r5);
    __m256 t6 = _mm256_unpacklo_ps(r6, r7);
    __m256 t7 = _mm256_unpackhi_ps(r6, r7);

    __m256 u0 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(1, 0, 1, 0));
    __m256 u1 = _mm256_shuffle_ps(t0, t2, _MM_SHUFFLE(3, 2, 3, 2));
    __m256 u2 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(1, 0, 1, 0));
    __m256 u3 = _mm256_shuffle_ps(t1, t3, _MM_SHUFFLE(3, 2, 3, 2));
    __m256 u4 = _mm256_shuffle_ps(t4, t6, _MM_SHUFFLE(1, 0, 1, 0));
    __m256 u5 = _mm256_shuffle_ps(t4, t6, _MM_SHUFFLE(3, 2, 3, 2));
    __m256 u6 = _mm256_shuffle_ps(t5, t7, _MM_SHUFFLE(1, 0, 1, 0));
    __m256 u7 = _mm256_shuffle_ps(t5, t7, _MM_SHUFFLE(3, 2, 3, 2));

    __m256 o0 = _mm256_permute2f128_ps(u0, u4, 0x20);
    __m256 o1 = _mm256_permute2f128_ps(u1, u5, 0x20);
    __m256 o2 = _mm256_permute2f128_ps(u2, u6, 0x20);
    __m256 o3 = _mm256_permute2f128_ps(u3, u7, 0x20);
    __m256 o4 = _mm256_permute2f128_ps(u0, u4, 0x31);
    __m256 o5 = _mm256_permute2f128_ps(u1, u5, 0x31);
    __m256 o6 = _mm256_permute2f128_ps(u2, u6, 0x31);
    __m256 o7 = _mm256_permute2f128_ps(u3, u7, 0x31);

    _mm256_storeu_ps(dst + 0 * dst_stride, o0);
    _mm256_storeu_ps(dst + 1 * dst_stride, o1);
    _mm256_storeu_ps(dst + 2 * dst_stride, o2);
    _mm256_storeu_ps(dst + 3 * dst_stride, o3);
    _mm256_storeu_ps(dst + 4 * dst_stride, o4);
    _mm256_storeu_ps(dst + 5 * dst_stride, o5);
    _mm256_storeu_ps(dst + 6 * dst_stride, o6);
    _mm256_storeu_ps(dst + 7 * dst_stride, o7);
}

// 前提：src 在原张量中是 F32 contiguous（按原行主序），要输出的 dst 是 src 的转置。
// 这里 src_row_elems 是原张量每行 F32 个数（= 输出的列数 cols），
// src_col_elems = 1（F32 元素相邻）。
// 该函数专门服务 2D transpose：输出 [rows, cols] 来自原 [cols, rows] contiguous。
inline void transpose_2d_f32_avx2(
    const float* src,      // 原连续矩阵（shape [cols, rows]）
    std::int64_t src_lead, // src 每行 F32 个数 = rows_out 方向 = "rows"
    float*       dst,      // 输出矩阵（shape [rows, cols] contiguous）
    std::int64_t rows_out, // 输出行数
    std::int64_t cols_out, // 输出列数
    std::int64_t dst_lead
) // dst 每行 F32 个数 = cols_out
{
    constexpr std::int64_t T       = 8;
    const std::int64_t     tiles_r = rows_out / T * T;
    const std::int64_t     tiles_c = cols_out / T * T;

    auto run_rows = [&](std::int64_t r0, std::int64_t r1) {
        std::int64_t r = r0;
        for (; r + T <= r1; r += T) {
            std::int64_t c = 0;
            for (; c + T <= cols_out; c += T) {
                // src[c..c+8, r..r+8] → dst[r..r+8, c..c+8]
                const float* sp = src + c * src_lead + r;
                float*       dp = dst + r * dst_lead + c;
                transpose_8x8_ps_avx2(sp, src_lead, dp, dst_lead);
            }
            // 尾列
            for (; c < cols_out; ++c) {
                for (std::int64_t rr = r; rr < r + T; ++rr) {
                    dst[rr * dst_lead + c] = src[c * src_lead + rr];
                }
            }
        }
        // 尾行
        for (; r < r1; ++r) {
            for (std::int64_t c = 0; c < cols_out; ++c)
                dst[r * dst_lead + c] = src[c * src_lead + r];
        }
    };
    (void)tiles_r;
    (void)tiles_c;

    if (rows_out < kTrParallelRows) {
        run_rows(0, rows_out);
        return;
    }
    const std::size_t grain = static_cast<std::size_t>(T * kTrGrainTileRows);
    parallel::parallel_for(std::size_t{0}, static_cast<std::size_t>(rows_out), grain, [&](std::size_t lo, std::size_t hi) {
        run_rows(static_cast<std::int64_t>(lo), static_cast<std::int64_t>(hi));
    });
}

#endif // BEE_SIMD_ENABLE_AVX2

// ── 顶层分派：按 elem_sz 与 ISA 选择实现 ────────────────────────────────────
// 接口：仅处理 2D 的 "strided-source → contiguous-dst" 拷贝；
//   src_row_stride_elems / src_col_stride_elems 以"元素"为单位。
template <typename ISA>
inline void cpu_transpose_2d_dispatch(
    const void*  src,
    void*        dst,
    std::int64_t rows,
    std::int64_t cols,
    std::int64_t src_row_stride_elems, // 输出行 → src 偏移
    std::int64_t src_col_stride_elems, // 输出列 → src 偏移
    std::size_t  elem_sz
)
{
    if (rows <= 0 || cols <= 0)
        return;

#if defined(BEE_SIMD_ENABLE_AVX2)
    // F32 专用 fast-path：src_row_stride_elems == 1（输出行方向在 src 中单位步进）
    // 意味着 src 是 [cols, rows] 连续、我们要转置到 [rows, cols]；
    //   src[r, c] = src_base[c * src_col_stride_elems + r]
    if constexpr (std::is_same_v<ISA, simd::IsaAvx2>) {
        if (elem_sz == sizeof(float) && src_row_stride_elems == 1) {
            transpose_2d_f32_avx2(static_cast<const float*>(src), src_col_stride_elems, static_cast<float*>(dst), rows, cols, cols);
            return;
        }
    }
#endif

    // 通用 blocked-tile 拷贝
    const std::int64_t src_row_bytes = src_row_stride_elems * static_cast<std::int64_t>(elem_sz);
    const std::int64_t src_col_bytes = src_col_stride_elems * static_cast<std::int64_t>(elem_sz);
    transpose_2d_generic(static_cast<const std::uint8_t*>(src), src_row_bytes, src_col_bytes, static_cast<std::uint8_t*>(dst), rows, cols, elem_sz);
}

} // namespace bee::cpu
