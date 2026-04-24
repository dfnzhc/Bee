/**
 * @File Cpu/Gemm/GemmDriver.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief AVX2/SSE2 共用的 Goto 三层分块 GEMM driver（支持外层 parallel_for）。
 *
 * 拓扑：
 *   for jc in 0..N_main by NC:
 *     for pc in 0..K by KC:
 *       pack B[pc..pc+kc, jc..jc+nc]                     —— 主线程串行
 *       parallel_for(chunk in 0..num_ic_chunks):         —— 并行 ic 方向
 *         thread_local A_pack[MC * KC]
 *         for each ic in chunk:
 *            pack A[ic..ic+mc, pc..pc+kc]
 *            for jr, ir: microkernel
 *
 *  并行度：num_ic_chunks = ceil(M_main / MC)，由 parallel_for 内部再按 grain 汇聚。
 *  阈值：M*N*K < kGemmParallelFlops 时走单线程路径，避免 fork-join 开销压过收益。
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Base/Parallel/ParallelFor.hpp"
#include "Tensor/Cpu/Gemm/GemmCommon.hpp"
#include "Tensor/Cpu/Gemm/PackCommon.hpp"

namespace bee::cpu::gemm::detail
{

// 并行阈值：约 4 M FLOPs（例如 128×128×128 ≈ 2 MF 走串行，256×256×256 ≈ 33 MF 走并行）。
inline constexpr std::int64_t kGemmParallelFlops = 4LL * 1024 * 1024;

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

// 返回当前线程用的 A_pack 缓冲区指针（thread_local，每线程 1 份，按需懒初始化）。
// 容量按 `(MC_MAX * KC_MAX * sizeof(std::int64_t))` 预留，足够覆盖所有 ISA × dtype 组合
// （AVX2 MC=192 KC=384 → 288 KB × 8B = 576 KB；SSE2 MC=128 KC=256 → 32 KB × 8B = 256 KB）。
inline auto thread_local_a_pack_buffer() -> void*
{
    static constexpr std::size_t kBytes = 192 * 384 * sizeof(std::int64_t); // 576 KB
    thread_local AlignedBuffer   buf(kBytes, 64);
    return buf.ptr;
}

// 单线程 driver（并行阈值之下、尾巴处理之前用）。
template <typename TA, typename TC, int MR, int NR, int MC, int KC, int NC, typename MicroK>
inline auto gemm_driver_serial(std::int64_t M, std::int64_t K, std::int64_t N, const TA* A, const TA* B, TC* C, MicroK micro) -> void
{
    const std::int64_t lda    = K;
    const std::int64_t ldb    = N;
    const std::int64_t ldc    = N;
    const std::int64_t M_main = (M / MR) * MR;
    const std::int64_t N_main = (N / NR) * NR;

    AlignedBuffer a_pack_buf(static_cast<std::size_t>(MC) * KC * sizeof(TA), 64);
    AlignedBuffer b_pack_buf(static_cast<std::size_t>(KC) * NC * sizeof(TA), 64);
    TA*           A_pack = a_pack_buf.template as<TA>();
    TA*           B_pack = b_pack_buf.template as<TA>();

    for (std::int64_t jc = 0; jc < N_main; jc += NC) {
        const std::int64_t nc = min_i<std::int64_t>(N_main - jc, NC);
        for (std::int64_t pc = 0; pc < K; pc += KC) {
            const std::int64_t kc = min_i<std::int64_t>(K - pc, KC);
            pack_B_nr<TA, NR>(B + pc * ldb + jc, ldb, kc, nc, B_pack);

            for (std::int64_t ic = 0; ic < M_main; ic += MC) {
                const std::int64_t mc = min_i<std::int64_t>(M_main - ic, MC);
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

    if (N_main < N) {
        scalar_gemm_add<TA, TC>(M, K, N - N_main, A, lda, B + N_main, ldb, C + N_main, ldc);
    }
    if (M_main < M && N_main > 0) {
        scalar_gemm_add<TA, TC>(M - M_main, K, N_main, A + M_main * lda, lda, B, ldb, C + M_main * ldc, ldc);
    }
}

// 通用 driver：超过并行阈值时在 ic 方向走 parallel_for，每个 worker 用 thread_local
// A_pack；否则退化到 serial 版本避免 fork-join 开销。
template <typename TA, typename TC, int MR, int NR, int MC, int KC, int NC, typename MicroK>
inline auto gemm_driver(std::int64_t M, std::int64_t K, std::int64_t N, const TA* A, const TA* B, TC* C, MicroK micro) -> void
{
    // 并行阈值判定
    if (M * K * N < kGemmParallelFlops) {
        gemm_driver_serial<TA, TC, MR, NR, MC, KC, NC>(M, K, N, A, B, C, micro);
        return;
    }

    const std::int64_t lda    = K;
    const std::int64_t ldb    = N;
    const std::int64_t ldc    = N;
    const std::int64_t M_main = (M / MR) * MR;
    const std::int64_t N_main = (N / NR) * NR;

    AlignedBuffer b_pack_buf(static_cast<std::size_t>(KC) * NC * sizeof(TA), 64);
    TA*           B_pack = b_pack_buf.template as<TA>();

    // M_main 方向按 MC 切 chunk；每个 chunk 一个 worker 负责，内部复用 thread_local A_pack
    const std::int64_t num_ic_chunks = (M_main + MC - 1) / MC;

    for (std::int64_t jc = 0; jc < N_main; jc += NC) {
        const std::int64_t nc = min_i<std::int64_t>(N_main - jc, NC);
        for (std::int64_t pc = 0; pc < K; pc += KC) {
            const std::int64_t kc = min_i<std::int64_t>(K - pc, KC);
            // 串行 pack B（共享）
            pack_B_nr<TA, NR>(B + pc * ldb + jc, ldb, kc, nc, B_pack);

            // 并行 ic 循环：grain=1（1 chunk/worker），num_ic_chunks 通常 6-16
            ::bee::parallel::parallel_for(
                std::size_t{0}, static_cast<std::size_t>(num_ic_chunks), std::size_t{1}, [&](std::size_t lo, std::size_t hi) {
                    TA* A_pack = static_cast<TA*>(thread_local_a_pack_buffer());
                    for (std::size_t ci = lo; ci < hi; ++ci) {
                        const std::int64_t ic = static_cast<std::int64_t>(ci) * MC;
                        const std::int64_t mc = min_i<std::int64_t>(M_main - ic, MC);
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
            );
        }
    }

    // 尾巴处理（单线程即可，占比 < 1%）
    if (N_main < N) {
        scalar_gemm_add<TA, TC>(M, K, N - N_main, A, lda, B + N_main, ldb, C + N_main, ldc);
    }
    if (M_main < M && N_main > 0) {
        scalar_gemm_add<TA, TC>(M - M_main, K, N_main, A + M_main * lda, lda, B, ldb, C + M_main * ldc, ldc);
    }
}

} // namespace bee::cpu::gemm::detail
