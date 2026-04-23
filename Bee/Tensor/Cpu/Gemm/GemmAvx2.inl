/**
 * @File Cpu/Gemm/GemmAvx2.inl
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief AVX2 GEMM 入口：委托 GemmDriver（带 parallel_for）+ AVX2 microkernel。
 *
 * 行主序约定：C[M,N] += A[M,K] · B[K,N]。C 已由调用方 memset 为 0。
 */

#include "Tensor/Cpu/Gemm/GemmCommon.hpp"
#include "Tensor/Cpu/Gemm/GemmDispatch.hpp"
#include "Tensor/Cpu/Gemm/GemmDriver.hpp"
#include "Tensor/Cpu/Gemm/KernelAvx2.hpp"

#include <cstdint>

namespace bee::cpu::gemm::avx2
{

auto gemm_f32(std::int64_t M, std::int64_t K, std::int64_t N, const float* A, const float* B, float* C) -> void
{
    using BS = Avx2BlockSize;
    ::bee::cpu::gemm::detail::gemm_driver<float, float, BS::MR, BS::NR_F, BS::MC, BS::KC, BS::NC>(M, K, N, A, B, C, &micro_kernel_sgemm_8x8);
}

auto gemm_f64(std::int64_t M, std::int64_t K, std::int64_t N, const double* A, const double* B, double* C) -> void
{
    using BS = Avx2BlockSize;
    ::bee::cpu::gemm::detail::gemm_driver<double, double, BS::MR, BS::NR_D, BS::MC, BS::KC, BS::NC>(M, K, N, A, B, C, &micro_kernel_dgemm_8x4);
}

auto gemm_i32(std::int64_t M, std::int64_t K, std::int64_t N, const std::int32_t* A, const std::int32_t* B, std::int32_t* C) -> void
{
    using BS = Avx2BlockSize;
    ::bee::cpu::gemm::detail::gemm_driver<std::int32_t, std::int32_t, BS::MR, BS::NR_F, BS::MC, BS::KC, BS::NC>(M, K, N, A, B, C, &micro_kernel_i32_8x8);
}

auto gemm_i8_i32(std::int64_t M, std::int64_t K, std::int64_t N, const std::int8_t* A, const std::int8_t* B, std::int32_t* C) -> void
{
    using BS = Avx2BlockSize;
    ::bee::cpu::gemm::detail::gemm_driver<std::int8_t, std::int32_t, BS::MR, BS::NR_I8, BS::MC, BS::KC, BS::NC>(M, K, N, A, B, C, &micro_kernel_i8_i32_8x8);
}

} // namespace bee::cpu::gemm::avx2
