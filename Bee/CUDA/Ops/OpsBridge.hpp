/**
 * @File Ops/OpsBridge.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief CUDA 算子内核与 host API 之间的 int 错误码桥接声明。
 *
 * 本头同时被 Api.cpp 与 Ops/*.cu 包含。所有指针参数都指向连续设备缓冲；
 * 函数返回 cudaError_t 的整数编码，Api.cpp 负责把它包装为 Bee::Result。
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace bee::cuda::detail
{

// ScalarType / BinaryOp / UnaryOp 的整数值必须与 Api.hpp 中的枚举保持一致。
// 使用 int 传参可以让 .cu 内核入口保持简单的 C 风格 ABI。

int ops_binary(int op, int dt, const void* a, const void* b, void* out, std::size_t n) noexcept;
int ops_unary(int op, int dt, const void* a, void* out, std::size_t n) noexcept;
int ops_cast(int src_dt, const void* src, int dst_dt, void* dst, std::size_t n) noexcept;

int ops_reduce_global(int op, int dt, const void* src, void* dst, std::size_t n) noexcept;
int ops_reduce_axis(int op, int dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) noexcept;

// 稳定 softmax：输入/输出均视为 [outer, axis, inner] 连续布局，dtype 仅 F32/F64。
int ops_softmax(int dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) noexcept;

// dt ∈ {F32, F64}；buf[i] *= factor（原地缩放，供 mean 使用）。
int ops_scale_fp(int dt, void* buf, double factor, std::size_t n) noexcept;

// 2D tiled-shared matmul：C[M,N] = A[M,K] * B[K,N]；A/B/C 同 dtype、均连续。
int ops_matmul(int dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) noexcept;
// 手写 TMA + WMMA TF32 GEMM 显式入口；仅支持 F32，要求 M%128==0、
// N%128==0、K%32==0 且 A/B/C 至少 16B 对齐。
int ops_matmul_force_tma_wmma(int dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) noexcept;

// 低精度 GEMM：读取 F16（dt=7）或 BF16（dt=8）输入，以 float 累加，输出 float。
// C 指向 float 设备缓冲（M×N 个 float）；A/B 指向对应低精度存储缓冲。
// 不支持的 dt 或有效尺寸下的空指针返回 cudaErrorInvalidValue。
// K==0 时函数直接返回成功且不写 C；调用方须保证 C 已预清零。
int ops_matmul_lowp(int dt, const void* A, const void* B, float* C, std::size_t M, std::size_t K, std::size_t N) noexcept;

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

// 设备侧 Philox4x32-10 随机数。
int ops_random_uniform(int dt, void* dst, std::size_t n, std::uint64_t seed) noexcept;
int ops_random_normal(int dt, void* dst, std::size_t n, std::uint64_t seed) noexcept;
int ops_random_int(int dt, void* dst, std::size_t n, std::int64_t low, std::int64_t high, std::uint64_t seed) noexcept;

// ── AI 基础原语 ──────────────────────────────────────────────────────────────
//
// RMSNorm：y[r,i] = x[r,i] / sqrt(mean(x[r]^2) + eps) * w[i]
// 输入/输出均为连续设备内存；rows × dim 布局；dt 仅支持 F32/F64。
int ops_rms_norm(int dt, const void* x, const void* w, void* out, std::size_t rows, std::size_t dim, double eps) noexcept;

// RoPE：split-half 配对旋转位置编码
// 输入布局：[n_batch, seq_len, dim]；dt 仅支持 F32/F64；dim 须为偶数。
int ops_rope(
    int          dt,
    const void*  x,
    void*        out,
    std::size_t  n_batch,
    std::size_t  seq_len,
    std::size_t  dim,
    double       base,
    std::int64_t position_offset
) noexcept;

// Embedding：按 ids 行取 weight；越界 id 在设备侧检测并返回 cudaErrorInvalidValue。
// weight_dt 仅 F32/F64；ids_dt 仅 I32/I64；vocab 须 > 0。
int ops_embedding(
    int         weight_dt,
    int         ids_dt,
    const void* weight,
    const void* ids,
    void*       out,
    std::size_t n_ids,
    std::size_t hidden,
    std::size_t vocab
) noexcept;

} // namespace bee::cuda::detail
