#pragma once

// Tensor CPU 算子运行期分派：每个 (算子, ISA) 组合提供一个非模板包装函数
// 实现在 Cpu/Dispatch/KernelDispatch.cpp，被 4 个 OBJECT library 各编译一次，
// 分别以 BEE_DISPATCH_ISA_{Scalar,Sse2,Avx2,Avx512} 宏选择命名空间与 ISA 标签

#include "Tensor/Core/Tensor.hpp"
#include "SIMD/Detect.hpp"

#include <cstdint>

namespace bee::cpu
{

// 每个 ISA 命名空间都声明同一组函数原型；
// 链接期选择由运行期 switch 决定
#define BEE_DECL_DISPATCH_NS(NS)                                                                                                            \
    namespace NS                                                                                                                            \
    {                                                                                                                                       \
        /* 二元 elementwise */                                                                                                              \
        auto ew_add(const Tensor& a, const Tensor& b, Tensor& out) -> void;                                                                 \
        auto ew_sub(const Tensor& a, const Tensor& b, Tensor& out) -> void;                                                                 \
        auto ew_mul(const Tensor& a, const Tensor& b, Tensor& out) -> void;                                                                 \
        auto ew_div(const Tensor& a, const Tensor& b, Tensor& out) -> void;                                                                 \
        /* 一元 elementwise */                                                                                                              \
        auto ew_neg(const Tensor& a, Tensor& out) -> void;                                                                                  \
        auto ew_abs(const Tensor& a, Tensor& out) -> void;                                                                                  \
        auto ew_sqrt(const Tensor& a, Tensor& out) -> void;                                                                                 \
        auto ew_exp(const Tensor& a, Tensor& out) -> void;                                                                                  \
        auto ew_log(const Tensor& a, Tensor& out) -> void;                                                                                  \
        /* 全局 reduce */                                                                                                                   \
        auto rd_sum_global(const Tensor& a, Tensor& out) -> void;                                                                           \
        auto rd_min_global(const Tensor& a, Tensor& out) -> void;                                                                           \
        auto rd_max_global(const Tensor& a, Tensor& out) -> void;                                                                           \
        auto rd_prod_global(const Tensor& a, Tensor& out) -> void;                                                                          \
        /* matmul（仅 F32/F64/I32/I64）*/                                                                                                   \
        auto mm_f32(std::int64_t M, std::int64_t K, std::int64_t N, const float* A, const float* B, float* C) -> void;                      \
        auto mm_f64(std::int64_t M, std::int64_t K, std::int64_t N, const double* A, const double* B, double* C) -> void;                   \
        auto mm_i32(std::int64_t M, std::int64_t K, std::int64_t N, const std::int32_t* A, const std::int32_t* B, std::int32_t* C) -> void; \
        auto mm_i64(std::int64_t M, std::int64_t K, std::int64_t N, const std::int64_t* A, const std::int64_t* B, std::int64_t* C) -> void; \
        auto mm_i8(std::int64_t M, std::int64_t K, std::int64_t N, const std::int8_t* A, const std::int8_t* B, std::int32_t* C) -> void;    \
    }

BEE_DECL_DISPATCH_NS(scalar)
BEE_DECL_DISPATCH_NS(sse2)
BEE_DECL_DISPATCH_NS(avx2)
BEE_DECL_DISPATCH_NS(avx512)

#undef BEE_DECL_DISPATCH_NS

// ─────────────────────────────────────────────────────────────────────────────
// 运行期分派宏：根据 simd::current_isa() 选择对应命名空间的函数
// 对本机不支持的 ISA，其 case 会被 BEE_TENSOR_HAVE_<ISA> 宏条件编译掉；
// 此时检测函数也不会返回该 ISA，直接落到 Scalar 兜底分支
// ─────────────────────────────────────────────────────────────────────────────

#if defined(BEE_TENSOR_HAVE_AVX512)
    #define BEE_RT_CASE_AVX512(FN, ...)                                        \
    case ::bee::simd::Isa::Avx512: return ::bee::cpu::avx512::FN(__VA_ARGS__);
#else
    #define BEE_RT_CASE_AVX512(FN, ...)
#endif

#if defined(BEE_TENSOR_HAVE_AVX2)
    #define BEE_RT_CASE_AVX2(FN, ...)                                      \
    case ::bee::simd::Isa::Avx2: return ::bee::cpu::avx2::FN(__VA_ARGS__);
#else
    #define BEE_RT_CASE_AVX2(FN, ...)
#endif

#if defined(BEE_TENSOR_HAVE_SSE2)
    #define BEE_RT_CASE_SSE2(FN, ...)                                      \
    case ::bee::simd::Isa::Sse2: return ::bee::cpu::sse2::FN(__VA_ARGS__);
#else
    #define BEE_RT_CASE_SSE2(FN, ...)
#endif

#define BEE_RT_DISPATCH(FN, ...)                    \
    do {                                            \
        switch (::bee::simd::current_isa()) {       \
            BEE_RT_CASE_AVX512(FN, __VA_ARGS__)     \
            BEE_RT_CASE_AVX2(FN, __VA_ARGS__)       \
            BEE_RT_CASE_SSE2(FN, __VA_ARGS__)       \
        default: break;                             \
        }                                           \
        return ::bee::cpu::scalar::FN(__VA_ARGS__); \
    } while (0)

} // namespace bee::cpu
