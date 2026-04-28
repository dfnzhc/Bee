/**
 * @File Api.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA 运行时桥接翻译单元。
 *
 * 本文件由 nvcc 编译，只负责直接调用 cuda_runtime.h 并返回简单的整数
 * 错误码。Api.cpp 再把这些错误码转换为 Bee::Result，避免在 nvcc 翻译
 * 单元中混用 <expected>、格式化库和更高层诊断类型。
 */

#include "CUDA/Runtime.hpp"

#include <cuda_runtime.h>

namespace bee::cuda::detail
{

int runtime_get_device_count(int* out) noexcept
{
    int        n   = 0;
    const auto err = cudaGetDeviceCount(&n);
    if (out)
        *out = (err == cudaSuccess) ? n : 0;
    if (err != cudaSuccess) {
        (void)cudaGetLastError();
    }
    return static_cast<int>(err);
}

int runtime_set_device(int device_index) noexcept
{
    return static_cast<int>(cudaSetDevice(device_index));
}

int runtime_device_synchronize() noexcept
{
    return static_cast<int>(cudaDeviceSynchronize());
}

const char* runtime_error_name(int err) noexcept
{
    return cudaGetErrorName(static_cast<cudaError_t>(err));
}

const char* runtime_error_string(int err) noexcept
{
    return cudaGetErrorString(static_cast<cudaError_t>(err));
}

bool runtime_compiled_with_nvcc() noexcept
{
#if defined(__CUDACC__)
    return true;
#else
    return false;
#endif
}

} // namespace bee::cuda::detail
