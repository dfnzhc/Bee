/**
 * @File Cpu/Gemm/PackCommon.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
 *
 * GEMM 的通用 A/B panel packing。
 *
 * 当前 AVX2 与 SSE2 的 packing 布局完全一致，因此提取为共享实现：
 *  - A pack：把 A 的 mc×kc 子块重排为 (mc / MR) 个条带，每条带布局为
 *    [(k=0 的 MR 个元素)(k=1 的 MR 个元素)...]，供微内核逐 k 广播 A 标量。
 *  - B pack：把 B 的 kc×nc 子块重排为 (nc / NR) 个条带，每条带布局为
 *    [(k=0 的 NR 个元素)(k=1 的 NR 个元素)...]，供微内核逐 k 连续加载 B 向量。
 *
 * 若后续某个 ISA 需要专门的交织、转置或预取友好布局，也统一在此文件中扩展。
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace bee::cpu::gemm
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

} // namespace bee::cpu::gemm
