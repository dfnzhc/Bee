/**
 * @File Runtime.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 连接 Api.cu 与 Api.cpp 的内部 C 风格桥接接口。
 *
 * 这些函数不是 Bee::CUDA 的公共 API。它们只在实现层把 nvcc 编译的
 * CUDA runtime 调用结果转换成简单的 int 或 const char*，再由 Api.cpp
 * 包装为 Bee::Result 与 Bee::Error。
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace bee::cuda::detail
{

// 返回 cudaError_t 的整数编码；0 表示 cudaSuccess。
int runtime_get_device_count(int* out) noexcept;
int runtime_set_device(int device_index) noexcept;
int runtime_device_synchronize() noexcept;

// 返回 CUDA runtime 持有的静态错误名称和错误描述字符串。
const char* runtime_error_name(int err) noexcept;
const char* runtime_error_string(int err) noexcept;

// 返回 Api.cu 翻译单元是否确实由 nvcc 编译。
bool runtime_compiled_with_nvcc() noexcept;

} // namespace bee::cuda::detail
