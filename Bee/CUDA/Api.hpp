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

} // namespace bee::cuda
