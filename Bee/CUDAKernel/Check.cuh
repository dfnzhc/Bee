/**
 * @File Check.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cuda_runtime.h>

#include <sstream>
#include <stdexcept>
#include <string>

namespace bee::cuda
{

inline void check_cuda(cudaError_t err, const char* expr, const char* file, int line)
{
    if (err == cudaSuccess) {
        return;
    }

    std::ostringstream oss;
    oss << "CUDA call failed: " << expr << "\n  file: " << file << ":" << line << "\n  code: " << static_cast<int>(err)
        << "\n  name: " << cudaGetErrorName(err) << "\n  what: " << cudaGetErrorString(err);
    throw std::runtime_error(oss.str());
}

inline void check_last_kernel(const char* file, int line)
{
    check_cuda(cudaGetLastError(), "cudaGetLastError()", file, line);
}

} // namespace bee::cuda

#define GPU_CUDA_CHECK(expr)  ::bee::cuda::check_cuda((expr), #expr, __FILE__, __LINE__)
#define GPU_CUDA_CHECK_LAST() ::bee::cuda::check_last_kernel(__FILE__, __LINE__)

// 可选 NVTX 标注（只有显式开启 GPU_ENABLE_NVTX 且可用头文件时生效）。
#if defined(BEE_ENABLE_NVTX)
    #if __has_include(<nvtx3/nvToolsExt.h>)
        #include <nvtx3/nvToolsExt.h>
        #define GPU_NVTX_RANGE_PUSH(msg) nvtxRangePushA(msg)
        #define GPU_NVTX_RANGE_POP()     nvtxRangePop()
    #else
        #define GPU_NVTX_RANGE_PUSH(msg) ((void)0)
        #define GPU_NVTX_RANGE_POP()     ((void)0)
    #endif
#else
    #define GPU_NVTX_RANGE_PUSH(msg) ((void)0)
    #define GPU_NVTX_RANGE_POP()     ((void)0)
#endif
