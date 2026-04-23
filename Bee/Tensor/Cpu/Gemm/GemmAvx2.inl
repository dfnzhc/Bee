/**
 * @File Cpu/Gemm/GemmAvx2.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
 *
 * AVX2 GEMM 驱动：三层分块（M_C, K_C, N_C）+ panel packing + register blocking。
 *
 * 行主序约定：C[M,N] += A[M,K] · B[K,N]。
 * 外层循环顺序（与参考 kernel 9 一致）：
 *   for jc in 0..N by N_C:
 *     for pc in 0..K by K_C:
 *       pack B[pc:pc+kc, jc:jc+nc] → B_pack
 *       for ic in 0..M by M_C:
 *         pack A[ic:ic+mc, pc:pc+kc] → A_pack
 *         for jr in 0..nc by NR:
 *           for ir in 0..mc by MR:
 *             micro_kernel(A_pack[ir], B_pack[jr], kc, &C[ic+ir, jc+jr], ldc)
 *
 * 尾巴（M/N 非 MR/NR 整数倍或 K 非倍数）：对边界条带用 scalar 兜底 —— 简单
 * 可靠，性能在大矩阵下被主干吞没。
 */

#include "Tensor/Cpu/Gemm/GemmCommon.hpp"
#include "Tensor/Cpu/Gemm/GemmDispatch.hpp"
#include "Tensor/Cpu/Gemm/KernelAvx2.hpp"
#include "Tensor/Cpu/Gemm/PackAvx2.hpp"

#include <cstdint>
#include <cstring>

namespace bee::cpu::gemm::avx2
{

namespace
{

    // 边界标量累加：处理 gemm 主干之外的余数行/列 / K 余数。
    template <typename TA, typename TC>
    inline auto scalar_gemm_add(
        std::int64_t M,
        std::int64_t K,
        std::int64_t N,
        const TA*    A,
        std::int64_t lda,
        const TA*    B,
        std::int64_t ldb,
        TC*          C,
        std::int64_t ldc
    ) -> void
    {
        for (std::int64_t i = 0; i < M; ++i) {
            for (std::int64_t k = 0; k < K; ++k) {
                const TC aik = static_cast<TC>(A[i * lda + k]);
                for (std::int64_t j = 0; j < N; ++j) {
                    C[i * ldc + j] += aik * static_cast<TC>(B[k * ldb + j]);
                }
            }
        }
    }

    // 通用驱动模板：参数 MR/NR、微内核函数指针通过包装函数传入。
    template <typename TA, typename TC, int MR, int NR, int MC, int KC, int NC, typename MicroK>
    inline auto gemm_driver(std::int64_t M, std::int64_t K, std::int64_t N, const TA* A, const TA* B, TC* C, MicroK micro) -> void
    {
        const std::int64_t lda = K;
        const std::int64_t ldb = N;
        const std::int64_t ldc = N;

        // 主干尺寸（MR/NR 的整数倍部分）
        const std::int64_t M_main = (M / MR) * MR;
        const std::int64_t N_main = (N / NR) * NR;

        AlignedBuffer a_pack_buf(static_cast<std::size_t>(MC) * KC * sizeof(TA), 64);
        AlignedBuffer b_pack_buf(static_cast<std::size_t>(KC) * NC * sizeof(TA), 64);
        TA*           A_pack = a_pack_buf.template as<TA>();
        TA*           B_pack = b_pack_buf.template as<TA>();

        // 外层：只驱动主干 M_main × N_main × K 的 GEMM；尾巴在函数末尾处理
        for (std::int64_t jc = 0; jc < N_main; jc += NC) {
            const auto nc = min_i<std::int64_t>(N_main - jc, NC);
            for (std::int64_t pc = 0; pc < K; pc += KC) {
                const auto kc = min_i<std::int64_t>(K - pc, KC);

                pack_B_nr<TA, NR>(B + pc * ldb + jc, ldb, kc, nc, B_pack);

                for (std::int64_t ic = 0; ic < M_main; ic += MC) {
                    const auto mc = min_i<std::int64_t>(M_main - ic, MC);
                    pack_A_mr<TA, MR>(A + ic * lda + pc, lda, mc, kc, A_pack);

                    for (std::int64_t jr = 0; jr < nc; jr += NR) {
                        const TA* Bp = B_pack + (jr / NR) * kc * NR;
                        for (std::int64_t ir = 0; ir < mc; ir += MR) {
                            const TA* Ap = A_pack + (ir / MR) * kc * MR;
                            micro(Ap, Bp, kc, C + (ic + ir) * ldc + (jc + jr), ldc);
                        }
                    }
                }
            }
        }

        // ── 尾巴处理 ────────────────────────────────────────────────────
        // 1) 右侧列尾: M=0..M, K=K, N=N_main..N  （宽度 N - N_main < NR）
        if (N_main < N) {
            scalar_gemm_add<TA, TC>(M, K, N - N_main, A, lda, B + N_main, ldb, C + N_main, ldc);
        }
        // 2) 底部行尾: M=M_main..M, K=K, N=N_main  （已排除右侧列尾）
        if (M_main < M && N_main > 0) {
            scalar_gemm_add<TA, TC>(M - M_main, K, N_main, A + M_main * lda, lda, B, ldb, C + M_main * ldc, ldc);
        }
    }

} // namespace

auto gemm_f32(std::int64_t M, std::int64_t K, std::int64_t N, const float* A, const float* B, float* C) -> void
{
    using BS = Avx2BlockSize;
    gemm_driver<float, float, BS::MR, BS::NR_F, BS::MC, BS::KC, BS::NC>(M, K, N, A, B, C, &micro_kernel_sgemm_8x8);
}

auto gemm_f64(std::int64_t M, std::int64_t K, std::int64_t N, const double* A, const double* B, double* C) -> void
{
    using BS = Avx2BlockSize;
    gemm_driver<double, double, BS::MR, BS::NR_D, BS::MC, BS::KC, BS::NC>(M, K, N, A, B, C, &micro_kernel_dgemm_8x4);
}

auto gemm_i32(std::int64_t M, std::int64_t K, std::int64_t N, const std::int32_t* A, const std::int32_t* B, std::int32_t* C) -> void
{
    using BS = Avx2BlockSize;
    gemm_driver<std::int32_t, std::int32_t, BS::MR, BS::NR_F, BS::MC, BS::KC, BS::NC>(M, K, N, A, B, C, &micro_kernel_i32_8x8);
}

auto gemm_i8_i32(std::int64_t M, std::int64_t K, std::int64_t N, const std::int8_t* A, const std::int8_t* B, std::int32_t* C) -> void
{
    using BS = Avx2BlockSize;
    gemm_driver<std::int8_t, std::int32_t, BS::MR, BS::NR_I8, BS::MC, BS::KC, BS::NC>(M, K, N, A, B, C, &micro_kernel_i8_i32_8x8);
}

} // namespace bee::cpu::gemm::avx2
