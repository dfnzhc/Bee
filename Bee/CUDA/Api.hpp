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

namespace ops
{

// n 以元素为单位；a/b/out 指向同一 dtype 的连续缓冲。
[[nodiscard]] auto binary(BinaryOp op, ScalarType dt, const void* a, const void* b, void* out, std::size_t n) -> Result<void>;

// 一元算子；src/dst 同 dtype、同 shape、均连续。
[[nodiscard]] auto unary(UnaryOp op, ScalarType dt, const void* src, void* dst, std::size_t n) -> Result<void>;

// dtype 转换；src/dst 均连续且元素数相同。
[[nodiscard]] auto cast(ScalarType src_dt, const void* src, ScalarType dst_dt, void* dst, std::size_t n) -> Result<void>;

} // namespace ops

} // namespace bee::cuda
