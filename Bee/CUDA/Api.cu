/**
 * @File Api.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA runtime bridge (nvcc-compiled TU).
 *
 * Provides thin wrappers over cuda_runtime.h so Api.cpp (C++23, MSVC)
 * can build Result<T> values without needing cuda_runtime.h or <expected>
 * interactions inside nvcc TU.
 *
 * ASCII-only: nvcc on Windows does not reliably accept non-ASCII sources.
 */

#include "CUDA/Runtime.hpp"

#include <cuda_runtime.h>

namespace bee::cuda::detail
{

int runtime_get_device_count(int* out) noexcept
{
    int n = 0;
    const auto err = cudaGetDeviceCount(&n);
    if (out) *out = (err == cudaSuccess) ? n : 0;
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
