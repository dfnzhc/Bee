/**
 * @File Core/Check.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief ASCII-only CUDA error-check macros for .cu translation units.
 *
 * Why two Check headers?
 *   - Core/Check.hpp : host-side, MSVC C++23, uses bee::Result<T>.
 *                     Include from .cpp only.
 *   - Core/Check.cuh : ASCII, nvcc C++20 safe, raw cudaError_t + int.
 *                     Include from .cu / .cuh only.
 *
 * nvcc on Windows cannot reliably accept <expected>/std::format_to_n,
 * and mis-decodes non-ASCII bytes in .cu source. So .cu code works with
 * raw integer error codes; host .cpp converts to bee::Error via
 * detail::runtime_error_{name,string}.
 */

#pragma once

#include <cuda_runtime.h>

// Execute expr. If it returns non-cudaSuccess, return its cudaError_t (cast to int).
// Caller function must have `int` return type (or compatible).
#define BEE_CUDA_RET_ON_ERR(expr)                                           \
    do {                                                                    \
        const cudaError_t _bee_cu_err = (expr);                             \
        if (_bee_cu_err != cudaSuccess) {                                   \
            (void)cudaGetLastError();                                       \
            return static_cast<int>(_bee_cu_err);                           \
        }                                                                   \
    } while (0)

// Launch a kernel (e.g. `kernel<<<g,b>>>(args)`) and return int on launch/last-error failure.
// Variadic so the `<<<...>>>` triple-angle and arg-list commas don't break macro parsing.
#define BEE_CUDA_LAUNCH_RET(...)                                            \
    do {                                                                    \
        __VA_ARGS__;                                                        \
        const cudaError_t _bee_cu_launch_err = cudaGetLastError();          \
        if (_bee_cu_launch_err != cudaSuccess) {                            \
            return static_cast<int>(_bee_cu_launch_err);                    \
        }                                                                   \
    } while (0)

// Return int (cudaSuccess == 0) from the current function.
#define BEE_CUDA_OK 0
