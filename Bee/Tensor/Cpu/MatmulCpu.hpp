#pragma once

// CPU matmul 内核：i-k-j 三重循环，F32/F64 内层使用 SIMD FMA 风格累加

#include "Tensor/Cpu/Simd/Simd.hpp"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace bee::cpu
{

// ─────────────────────────────────────────────────────────────────────────────
// SIMD 可用性 trait：标记哪些 (类型, ISA) 组合支持 matmul SIMD 内层
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, typename ISA> inline constexpr bool kSimdMatmul = false;

// IsaScalar 对 float/double 均可用（退化为标量宽度 1，与下方统一路径兼容）
template <> inline constexpr bool kSimdMatmul<float,  simd::IsaScalar> = true;
template <> inline constexpr bool kSimdMatmul<double, simd::IsaScalar> = true;

#ifdef BEE_TENSOR_SIMD_AVX2
// AVX2 仅为 float/double 提供 SIMD；整数类型走标量路径
template <> inline constexpr bool kSimdMatmul<float,  simd::IsaAvx2> = true;
template <> inline constexpr bool kSimdMatmul<double, simd::IsaAvx2> = true;
#endif

// ─────────────────────────────────────────────────────────────────────────────
// cpu_matmul_kernel：对已连续的原始指针执行 i-k-j matmul
//   A : [M, K]，B : [K, N]，C : [M, N]（调用者负责预先 memset 为 0）
// ─────────────────────────────────────────────────────────────────────────────

template <typename T>
auto cpu_matmul_kernel(
    int64_t M, int64_t K, int64_t N,
    const T* A,
    const T* B,
    T*       C) -> void
{
    using ISA = simd::DefaultIsa;

    // F32/F64 走 SIMD 内层（宽度 >= 1，自动涵盖 Scalar 降级）
    if constexpr (kSimdMatmul<T, ISA>) {
        using SB = simd::SimdBackend<T, ISA>;
        constexpr auto W = static_cast<int64_t>(SB::width);

        for (int64_t i = 0; i < M; ++i) {
            for (int64_t k = 0; k < K; ++k) {
                // 广播 a[i,k] 到 SIMD 寄存器
                auto va = SB::set1(A[i * K + k]);
                int64_t j = 0;
                // 向量化处理宽度对齐的部分
                for (; j + W <= N; j += W) {
                    auto vb = SB::loadu(B + k * N + j);
                    auto vc = SB::loadu(C + i * N + j);
                    // FMA 风格：vc += va * vb
                    vc = SB::add(vc, SB::mul(va, vb));
                    SB::storeu(C + i * N + j, vc);
                }
                // 尾部标量处理
                for (; j < N; ++j)
                    C[i * N + j] += A[i * K + k] * B[k * N + j];
            }
        }
    }
    else {
        // I32/I64：纯标量三重循环
        for (int64_t i = 0; i < M; ++i) {
            for (int64_t k = 0; k < K; ++k) {
                T a_ik = A[i * K + k];
                for (int64_t j = 0; j < N; ++j)
                    C[i * N + j] += a_ik * B[k * N + j];
            }
        }
    }
}

} // namespace bee::cpu
