/**
 * @File Cpu/Gemm/GemmDispatch.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief CPU GEMM 运行时分派入口声明。
 *
 * 四个类型 × 三个 ISA 的 GEMM 入口函数声明。
 * 行主序，C += A·B（调用者负责 C 的初始化 / 清零）。
 * I8GEMM：A/B 为 int8，C 为 int32（累加到 int32，输出 dtype=I32）。
 * I64 未提供 SIMD 版本。
 */

#pragma once

#include <cstdint>

namespace bee::cpu::gemm
{

#define BEE_GEMM_DECL_NS(NS)                                                                                                                   \
    namespace NS                                                                                                                               \
    {                                                                                                                                          \
        auto gemm_f32(std::int64_t M, std::int64_t K, std::int64_t N, const float* A, const float* B, float* C) -> void;                       \
        auto gemm_f64(std::int64_t M, std::int64_t K, std::int64_t N, const double* A, const double* B, double* C) -> void;                    \
        auto gemm_i32(std::int64_t M, std::int64_t K, std::int64_t N, const std::int32_t* A, const std::int32_t* B, std::int32_t* C) -> void;  \
        auto gemm_i8_i32(std::int64_t M, std::int64_t K, std::int64_t N, const std::int8_t* A, const std::int8_t* B, std::int32_t* C) -> void; \
    }

BEE_GEMM_DECL_NS(scalar)
BEE_GEMM_DECL_NS(sse2)
BEE_GEMM_DECL_NS(avx2)

#undef BEE_GEMM_DECL_NS

} // namespace bee::cpu::gemm
