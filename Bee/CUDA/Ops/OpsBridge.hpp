/**
 * @File Ops/OpsBridge.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief ASCII-only int-returning bridges for element-wise / cast kernels.
 *
 * Consumed by both Api.cpp (MSVC, Result-wrapping) and Ops/xx.cu (nvcc).
 * All parameters are raw pointers to device-contiguous buffers; n is the
 * number of elements.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace bee::cuda::detail
{

// ScalarType / BinaryOp / UnaryOp values must mirror the enums in Api.hpp.
// They are passed as plain ints to keep this header nvcc-friendly.

int ops_binary(int op, int dt, const void* a, const void* b, void* out, std::size_t n) noexcept;
int ops_unary(int op, int dt, const void* a, void* out, std::size_t n) noexcept;
int ops_cast(int src_dt, const void* src, int dst_dt, void* dst, std::size_t n) noexcept;

int ops_reduce_global(int op, int dt, const void* src, void* dst, std::size_t n) noexcept;
int ops_reduce_axis(int op, int dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) noexcept;

// dt ∈ {F32, F64}；buf[i] *= factor（原地缩放，供 mean 使用）。
int ops_scale_fp(int dt, void* buf, double factor, std::size_t n) noexcept;

// 2D tiled-shared matmul：C[M,N] = A[M,K] * B[K,N]；A/B/C 同 dtype、均连续。
int ops_matmul(int dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) noexcept;
// B10：手写 TMA + WMMA TF32 GEMM 显式入口（仅 F32，要求 M%128==N%128==K%32==0、16B 对齐）。
int ops_matmul_force_tma_wmma(int dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) noexcept;

// 2D tiled-shared transpose：dst[i,j] = src[j,i]，src 为 [rows, cols] 连续。
int ops_transpose_2d(int dt, const void* src, void* dst, std::size_t rows, std::size_t cols) noexcept;

// 通用 strided copy：将任意步长布局的 src（base + offset）物化到连续 dst。
// shape/strides 为 host 侧指针，ndim 最多 8；offset_elements 为 storage 基地址起的元素偏移。
int ops_strided_copy(
    int            dt,
    const void*    src,
    void*          dst,
    const int64_t* shape,
    const int64_t* strides,
    int            ndim,
    int64_t        offset_elements,
    std::size_t    numel
) noexcept;

// 设备侧 Philox4x32-10 随机数（B7）。
int ops_random_uniform(int dt, void* dst, std::size_t n, std::uint64_t seed) noexcept;
int ops_random_normal(int dt, void* dst, std::size_t n, std::uint64_t seed) noexcept;
int ops_random_int(int dt, void* dst, std::size_t n, std::int64_t low, std::int64_t high, std::uint64_t seed) noexcept;

} // namespace bee::cuda::detail
