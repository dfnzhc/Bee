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

// ── 内存（M2 实现） ──────────────────────────────────────────────────────────
//
// 以下接口将在 M2 里程碑中实装；目前为占位声明，当 CUDA 组件链接进 Tensor
// 时由 Bee::Tensor 的 Cuda/Backend.cpp 转发调用。

[[nodiscard]] auto allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*>;

void deallocate(void* ptr, std::size_t nbytes, std::size_t alignment) noexcept;

[[nodiscard]] auto memcpy_h2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>;
[[nodiscard]] auto memcpy_d2h(void* dst, const void* src, std::size_t nbytes) -> Result<void>;
[[nodiscard]] auto memcpy_d2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>;

} // namespace bee::cuda
