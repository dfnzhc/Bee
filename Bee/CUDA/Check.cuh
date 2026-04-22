/**
 * @File Check.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA internal error-check helpers (used from .cu sources).
 *
 * Design:
 * - Public APIs (see Api.hpp) always use Result<T>, never throw;
 * - Inside .cu sources, BEE_CUDA_CHECK maps cudaError_t into bee::Error;
 * - NVTX annotations are optional (enabled by BEE_ENABLE_NVTX).
 *
 * ASCII-only: nvcc on Windows does not reliably accept non-ASCII sources.
 */

#pragma once

#include <cuda_runtime.h>

#include <format>
#include <string_view>

#include "Base/Diagnostics/Error.hpp"

namespace bee::cuda::detail
{

[[nodiscard]] inline auto to_error(cudaError_t err, std::string_view expr) -> Error
{
    return make_error(
        std::format("CUDA call failed: {} : {} ({})", expr, cudaGetErrorName(err), cudaGetErrorString(err)),
        Severity::Recoverable,
        static_cast<int>(err)
    );
}

} // namespace bee::cuda::detail

// If expr returns non-cudaSuccess, return std::unexpected(Error) from the caller.
// The caller must be a function returning Result<T>.
#define BEE_CUDA_CHECK(expr)                                                                                                                \
    do {                                                                                                                                    \
        const cudaError_t _bee_cuda_err = (expr);                                                                                           \
        if (_bee_cuda_err != cudaSuccess) {                                                                                                 \
            return std::unexpected(::bee::cuda::detail::to_error(_bee_cuda_err, #expr));                                                    \
        }                                                                                                                                   \
    } while (0)

#define BEE_CUDA_CHECK_LAST() BEE_CUDA_CHECK(cudaGetLastError())

#if defined(BEE_ENABLE_NVTX)
    #if __has_include(<nvtx3/nvToolsExt.h>)
        #include <nvtx3/nvToolsExt.h>
        #define BEE_NVTX_RANGE_PUSH(msg) nvtxRangePushA(msg)
        #define BEE_NVTX_RANGE_POP()     nvtxRangePop()
    #else
        #define BEE_NVTX_RANGE_PUSH(msg) ((void)0)
        #define BEE_NVTX_RANGE_POP()     ((void)0)
    #endif
#else
    #define BEE_NVTX_RANGE_PUSH(msg) ((void)0)
    #define BEE_NVTX_RANGE_POP()     ((void)0)
#endif
