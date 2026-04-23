/**
 * @File Cpu/Gemm/PackSse2.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
 *
 * SSE2 GEMM 的 A/B panel packing。语义与 AVX2 版一致，MR/NR 更小。
 */

#pragma once

#include <cstdint>
#include <cstring>

namespace bee::cpu::gemm::sse2
{

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

} // namespace bee::cpu::gemm::sse2
