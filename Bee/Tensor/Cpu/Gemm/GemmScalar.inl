/**
 * @File Cpu/Gemm/GemmScalar.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
 *
 * 标量兜底 GEMM：朴素 i-k-j 三重循环 + 分块，不依赖任何 SIMD。
 * 对非 SSE2 机器以及单元测试基线提供可参考实现。
 */

#include "Tensor/Cpu/Gemm/GemmDispatch.hpp"

#include <cstdint>

namespace bee::cpu::gemm::scalar
{

namespace
{

    template <typename TA, typename TC>
    inline auto mm_kernel_ikj(std::int64_t M, std::int64_t K, std::int64_t N, const TA* A, const TA* B, TC* C) -> void
    {
        for (std::int64_t i = 0; i < M; ++i) {
            for (std::int64_t k = 0; k < K; ++k) {
                const TC a_ik = static_cast<TC>(A[i * K + k]);
                for (std::int64_t j = 0; j < N; ++j) {
                    C[i * N + j] += a_ik * static_cast<TC>(B[k * N + j]);
                }
            }
        }
    }

} // namespace

auto gemm_f32(std::int64_t M, std::int64_t K, std::int64_t N, const float* A, const float* B, float* C) -> void
{
    mm_kernel_ikj<float, float>(M, K, N, A, B, C);
}

auto gemm_f64(std::int64_t M, std::int64_t K, std::int64_t N, const double* A, const double* B, double* C) -> void
{
    mm_kernel_ikj<double, double>(M, K, N, A, B, C);
}

auto gemm_i32(std::int64_t M, std::int64_t K, std::int64_t N, const std::int32_t* A, const std::int32_t* B, std::int32_t* C) -> void
{
    mm_kernel_ikj<std::int32_t, std::int32_t>(M, K, N, A, B, C);
}

auto gemm_i8_i32(std::int64_t M, std::int64_t K, std::int64_t N, const std::int8_t* A, const std::int8_t* B, std::int32_t* C) -> void
{
    mm_kernel_ikj<std::int8_t, std::int32_t>(M, K, N, A, B, C);
}

} // namespace bee::cpu::gemm::scalar
