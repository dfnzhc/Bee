#pragma once

#include "Base/Diagnostics/Error.hpp"

#include <cstddef>
#include <cstdint>

namespace bee::tensor::cuda
{

// CUDA 后端接口声明
// 这些函数定义了 Bee::Tensor 与 CUDA 计算后端之间的边界。
// 当前实现均返回 NotImplemented 错误；
// 将来由 Bee::CUDA 组件提供真正的 CUDA 实现。

// 从设备内存分配指定字节数。
// @param nbytes: 要分配的字节数
// @param alignment: 对齐方式（字节数）
// @return 成功时返回分配的设备指针；失败时返回 NotImplemented 错误
[[nodiscard]] auto allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*>;

// 释放之前分配的设备内存。
// @param p: 设备指针
// @param nbytes: 分配时的字节数
// @param alignment: 分配时的对齐方式
auto deallocate(void* p, std::size_t nbytes, std::size_t alignment) -> void;

// 从主机内存复制数据到设备内存。
// @param dst: 设备目标指针
// @param src: 主机源指针
// @param nbytes: 要复制的字节数
// @return 成功时返回 void；失败时返回 NotImplemented 错误
[[nodiscard]] auto memcpy_h2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>;

// 从设备内存复制数据到主机内存。
// @param dst: 主机目标指针
// @param src: 设备源指针
// @param nbytes: 要复制的字节数
// @return 成功时返回 void；失败时返回 NotImplemented 错误
[[nodiscard]] auto memcpy_d2h(void* dst, const void* src, std::size_t nbytes) -> Result<void>;

// 在设备内存中复制数据（设备到设备）。
[[nodiscard]] auto memcpy_d2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>;

// 把设备内存 ptr 的前 nbytes 个字节置为 value 的低 8 位。
[[nodiscard]] auto memset(void* ptr, int value, std::size_t nbytes) -> Result<void>;

// 元素级二元算子（a、b、out 均为同 dtype、同长度、连续的设备内存）。
// op/dt 取值对齐 bee::cuda::BinaryOp / bee::cuda::ScalarType 的整型编码，
// 亦即 bee::DType 的整型编码（见 Tensor/Core/DType.hpp）。
[[nodiscard]] auto ew_binary(int op, int dt, const void* a, const void* b, void* out, std::size_t n) -> Result<void>;

// 元素级一元算子（src/dst 同 dtype、同长度、连续）。
[[nodiscard]] auto ew_unary(int op, int dt, const void* src, void* dst, std::size_t n) -> Result<void>;

// dtype 转换（src/dst 连续，元素数相同）。
[[nodiscard]] auto ew_cast(int src_dt, const void* src, int dst_dt, void* dst, std::size_t n) -> Result<void>;

// 全局 reduce：src 为连续 n 元素缓冲，dst 指向 1 个同 dtype 标量。
[[nodiscard]] auto reduce_global(int op, int dt, const void* src, void* dst, std::size_t n) -> Result<void>;

// 按轴 reduce：[outer, axis, inner] -> [outer, inner]，同 dtype、均连续。
[[nodiscard]] auto reduce_axis(int op, int dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) -> Result<void>;

// F32/F64 原地缩放（mean 使用）。
[[nodiscard]] auto scale_fp(int dt, void* buf, double factor, std::size_t n) -> Result<void>;

// 2D tiled-shared matmul：C[M,N] = A[M,K] * B[K,N]；同 dtype、连续。
[[nodiscard]] auto matmul(int dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) -> Result<void>;

// 2D tiled-shared transpose：dst[i,j] = src[j,i]。
[[nodiscard]] auto transpose_2d(int dt, const void* src, void* dst, std::size_t rows, std::size_t cols) -> Result<void>;

// 设备侧随机数填充（Philox4x32-10）。dtype 仅 F32/F64。
[[nodiscard]] auto random_uniform(int dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>;
[[nodiscard]] auto random_normal(int dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>;

// 整数区间 [low, high)；dtype ∈ {U8, I32, I64}。
[[nodiscard]] auto random_int(int dt, void* dst, std::size_t n, std::int64_t low, std::int64_t high, std::uint64_t seed) -> Result<void>;

// 同步 CUDA 设备，等待所有待处理操作完成。
// @return 成功时返回 void；失败时返回 NotImplemented 错误
[[nodiscard]] auto synchronize() -> Result<void>;

} // namespace bee::tensor::cuda
