#pragma once

#include "Base/Diagnostics/Error.hpp"

#include <cstddef>

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
// @param dst: 设备目标指针
// @param src: 设备源指针
// @param nbytes: 要复制的字节数
// @return 成功时返回 void；失败时返回 NotImplemented 错误
[[nodiscard]] auto memcpy_d2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>;

// 同步 CUDA 设备，等待所有待处理操作完成。
// @return 成功时返回 void；失败时返回 NotImplemented 错误
[[nodiscard]] auto synchronize() -> Result<void>;

} // namespace bee::tensor::cuda
