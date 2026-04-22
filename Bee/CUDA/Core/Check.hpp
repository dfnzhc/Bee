/**
 * @File Core/Check.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Host 端 CUDA 错误检查：返回 bee::Result<T>。
 *
 * 仅由 .cpp 源文件包含；需要 cuda_runtime.h（host side 可用）。
 * 对 .cu 源请使用 Core/Check.cuh（int 版本）。
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
        std::format("CUDA 调用失败: {} : {} ({})", expr, cudaGetErrorName(err), cudaGetErrorString(err)), Severity::Recoverable, static_cast<int>(err)
    );
}

} // namespace bee::cuda::detail

// expr 必须返回 cudaError_t。若失败则返回 std::unexpected(Error)。
// 调用者所在函数必须返回 bee::Result<T>。
#define BEE_CUDA_CHECK(expr)                                                             \
    do {                                                                                 \
        const cudaError_t _bee_cuda_err = (expr);                                        \
        if (_bee_cuda_err != cudaSuccess) {                                              \
            (void)cudaGetLastError();                                                    \
            return std::unexpected(::bee::cuda::detail::to_error(_bee_cuda_err, #expr)); \
        }                                                                                \
    } while (0)

#define BEE_CUDA_CHECK_LAST() BEE_CUDA_CHECK(cudaGetLastError())
