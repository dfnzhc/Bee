/**
 * @File Cpu/Gemm/PackAvx2.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
 *
 * AVX2 GEMM 的 A/B panel packing。
 *
 * 行主序视角下：
 *  - A: [M,K]，行距 lda=K。micro-kernel 需要 "MR 元素 / K 维度列"，
 *    pack_A 把 A 的 mc×kc 子块重排为 (mc/MR) 个条带，每条带布局
 *    [(k=0 MR 元素)(k=1 MR 元素)...(k=kc-1 MR 元素)] —— MR 个 row 元素在列上
 *    相邻，读取时用 gather 或逐行 load。我们选逐行 load（简单稳定）。
 *  - B: [K,N]，行距 ldb=N。micro-kernel 需要 "NR 元素 / K 维度行"，即每行连续 NR 个。
 *    pack_B 把 B 的 kc×nc 子块重排为 (nc/NR) 个条带，每条带布局
 *    [(k=0 NR 元素)(k=1 NR 元素)...]。由于源行已按 N 连续，直接 memcpy 即可。
 *
 * 对于非 MR/NR 整数倍的余数列/行，分别在外层驱动中做标量处理。
 * 这里的 packing 只处理整数倍部分。
 */

#pragma once

#include <immintrin.h>

#include <cstdint>
#include <cstring>

namespace bee::cpu::gemm::avx2
{

// ── A pack（通用模板 T，MR 模板参数）──────────────────────────────────────
// 源：A[m0..m0+mc, k0..k0+kc]，行距 lda
// 目标：dst，连续 (mc / MR) 条带 × (kc*MR 元素)
// 要求 mc 能被 MR 整除；调用方确保或自行处理余数。
template <typename T, int MR>
inline auto pack_A_mr(const T* __restrict A, std::int64_t lda, std::int64_t mc, std::int64_t kc, T* __restrict dst) -> void
{
    T* out = dst;
    for (std::int64_t mi = 0; mi + MR <= mc; mi += MR) {
        const T* a_base = A + mi * lda;
        for (std::int64_t k = 0; k < kc; ++k) {
#pragma unroll
            for (int r = 0; r < MR; ++r) {
                out[r] = a_base[r * lda + k];
            }
            out += MR;
        }
    }
}

// ── B pack（通用模板 T，NR 模板参数）──────────────────────────────────────
// 源：B[k0..k0+kc, n0..n0+nc]，行距 ldb
// 目标：dst，连续 (nc / NR) 条带 × (kc*NR 元素)
// 要求 nc 能被 NR 整除。
template <typename T, int NR>
inline auto pack_B_nr(const T* __restrict B, std::int64_t ldb, std::int64_t kc, std::int64_t nc, T* __restrict dst) -> void
{
    T* out = dst;
    for (std::int64_t nj = 0; nj + NR <= nc; nj += NR) {
        const T* b_base = B + nj;
        for (std::int64_t k = 0; k < kc; ++k) {
            std::memcpy(out, b_base + k * ldb, static_cast<std::size_t>(NR) * sizeof(T));
            out += NR;
        }
    }
}

} // namespace bee::cpu::gemm::avx2
