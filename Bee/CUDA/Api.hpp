/**
 * @File Api.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA 组件对外 C++ 接口（不暴露 cuda_runtime.h）。
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Base/Diagnostics/Error.hpp"

namespace bee::cuda
{

// ── 组件元信息 ──────────────────────────────────────────────────────────────

// 组件名（稳定字符串字面量）
[[nodiscard]] std::string_view component_name() noexcept;

// CUDA Toolkit 版本字符串（来自 CMake 探测）
[[nodiscard]] std::string_view toolkit_version() noexcept;

// 是否检测到 CUTLASS
[[nodiscard]] bool has_cutlass() noexcept;

// 是否由 nvcc 编译（从 .cu 单元观察）
[[nodiscard]] bool compiled_with_nvcc() noexcept;

// ── 设备查询 ────────────────────────────────────────────────────────────────

// 设备数量；若驱动不可用或无设备则返回 0
[[nodiscard]] auto device_count() noexcept -> int;

// 将当前线程绑定到指定设备
[[nodiscard]] auto set_device(int device_index) -> Result<void>;

// 阻塞直到当前设备上所有已提交工作完成
[[nodiscard]] auto device_synchronize() -> Result<void>;

// ── 内存 ────────────────────────────────────────────────────────────────────
//
// 默认使用当前 device 的 cudaMemPool_t（cudaMallocFromPoolAsync）与
// cudaStreamPerThread；每个入口在返回前都 cudaStreamSynchronize，对调用方
// 表现为同步 API（plan-cuda §5）。alignment 参数当前被忽略（cudaMallocAsync
// 保证的对齐 >= 256B 通常足够；M6/M7 若需特殊对齐再扩展）。

[[nodiscard]] auto allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*>;

void deallocate(void* ptr, std::size_t nbytes, std::size_t alignment) noexcept;

// pinned host memory (页锁定内存，提升 H2D/D2H 传输速度)
[[nodiscard]] auto allocate_pinned_host(std::size_t nbytes) -> Result<void*>;
auto free_pinned_host(void* ptr) noexcept -> void;

[[nodiscard]] auto memcpy_h2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>;
[[nodiscard]] auto memcpy_d2h(void* dst, const void* src, std::size_t nbytes) -> Result<void>;
[[nodiscard]] auto memcpy_d2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>;

// 把设备内存 ptr 的前 nbytes 个字节置为 value 的低 8 位。
[[nodiscard]] auto memset(void* ptr, int value, std::size_t nbytes) -> Result<void>;

// ── 元素级算子（M3） ────────────────────────────────────────────────────────
//
// 计算模型：输入/输出均为连续设备内存；CUDA 端暂不处理广播与非连续 stride，
// 由 Tensor 层在必要时先 contiguous() 再进入。ScalarType 与 bee::DType
// 共享同一整型编码（见 Tensor/Core/DType.hpp），便于直接转换。

enum class ScalarType : std::uint8_t
{
    Bool = 0,
    U8   = 1,
    I32  = 2,
    I64  = 3,
    F32  = 4,
    F64  = 5,
};

enum class BinaryOp : std::uint8_t
{
    Add,
    Sub,
    Mul,
    Div,
};

enum class UnaryOp : std::uint8_t
{
    Neg,
    Abs,
    Sqrt,
    Exp,
    Log,
};

enum class ReduceOp : std::uint8_t
{
    Sum,
    Min,
    Max,
    Prod,
};

namespace ops
{

    // n 以元素为单位；a/b/out 指向同一 dtype 的连续缓冲。
    [[nodiscard]] auto binary(BinaryOp op, ScalarType dt, const void* a, const void* b, void* out, std::size_t n) -> Result<void>;

    // 一元算子；src/dst 同 dtype、同 shape、均连续。
    [[nodiscard]] auto unary(UnaryOp op, ScalarType dt, const void* src, void* dst, std::size_t n) -> Result<void>;

    // dtype 转换；src/dst 均连续且元素数相同。
    [[nodiscard]] auto cast(ScalarType src_dt, const void* src, ScalarType dst_dt, void* dst, std::size_t n) -> Result<void>;

    // 全局 reduce：src 为连续 n 元素缓冲，dst 指向 1 个同 dtype 标量。
    [[nodiscard]] auto reduce_global(ReduceOp op, ScalarType dt, const void* src, void* dst, std::size_t n) -> Result<void>;

    // 按轴 reduce：输入视为 [outer, axis, inner] 三维连续布局；输出 [outer, inner]。
    // src/dst 同 dtype、均连续；axis 必须 >=1。
    [[nodiscard]] auto reduce_axis(ReduceOp op, ScalarType dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner)
        -> Result<void>;

    // 原地缩放（dt ∈ {F32, F64}）：供 mean 实现。
    [[nodiscard]] auto scale_fp(ScalarType dt, void* buf, double factor, std::size_t n) -> Result<void>;

    // 2D tiled-shared matmul：C[M,N] = A[M,K] * B[K,N]；A/B/C 同 dtype、均连续。
    [[nodiscard]] auto matmul(ScalarType dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) -> Result<void>;

    // 2D tiled-shared transpose：dst[i,j] = src[j,i]，src 为 [rows, cols] 连续。
    [[nodiscard]] auto transpose_2d(ScalarType dt, const void* src, void* dst, std::size_t rows, std::size_t cols) -> Result<void>;

    // ── 设备侧随机数（B7） ──────────────────────────────────────────────────────
    // Philox4x32-10 on-device：同一 seed 与 numel 下输出确定；dtype 仅支持 F32/F64。
    [[nodiscard]] auto random_uniform(ScalarType dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>;

    // 正态 N(0,1)；dtype 仅支持 F32/F64；内部以 Box-Muller 转换。
    [[nodiscard]] auto random_normal(ScalarType dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>;

    // 整数区间 [low, high)；dtype 支持 U8/I32/I64。
    [[nodiscard]] auto random_int(ScalarType dt, void* dst, std::size_t n, std::int64_t low, std::int64_t high, std::uint64_t seed) -> Result<void>;

    // ── Matmul 后端切换 ─────────────────────────────────────────────────────────
    //
    // Auto：按当前构建与当前设备能力自动选择可用实现。
    // Wmma：通用 WMMA / baseline 路径，sm_70 及以上可用。
    // Cutlass：依赖 CUTLASS 且要求当前设备具备对应矩阵乘能力。
    // Native：当前指 Blackwell 专用的 TMA + WMMA F32 路径。
    enum class MatmulBackend : uint8_t
    {
        Auto    = 0,
        Wmma    = 1,
        Cutlass = 2,
        Native  = 3,
    };

    // 设置全局默认 matmul 后端；返回旧值。线程安全（std::atomic）。
    auto set_matmul_backend(MatmulBackend backend) noexcept -> MatmulBackend;

    // 查询当前全局默认 matmul 后端。
    [[nodiscard]] auto get_matmul_backend() noexcept -> MatmulBackend;

    // 查询后端在“当前构建 + 当前设备”组合下是否可用。
    // 返回 false 代表当前构建未包含所需实现，或当前设备不具备该后端要求的硬件能力。
    [[nodiscard]] auto matmul_backend_available(MatmulBackend backend) noexcept -> bool;

} // namespace ops

// ── 异步运行时 API ──────────────────────────────────────────────────────────

// 从句柄获取 CUDA stream（骨架阶段直接传递）。
[[nodiscard]] auto stream_from_handle(void* handle) -> Result<void*>;

// 异步内存拷贝（接受用户提供的 stream 句柄）。
[[nodiscard]] auto memcpy_h2d_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>;
[[nodiscard]] auto memcpy_d2h_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>;
[[nodiscard]] auto memcpy_d2d_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>;

// 事件 API。
[[nodiscard]] auto create_event() -> Result<void*>;
[[nodiscard]] auto record_event(void* event_handle, void* stream) -> Result<void>;
[[nodiscard]] auto wait_event(void* event_handle, void* stream) -> Result<void>;

// workspace 管理。
[[nodiscard]] auto request_workspace(std::size_t nbytes, void* stream) -> Result<void*>;

} // namespace bee::cuda
