/**
 * @File Core/Launch.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Kernel launch 辅助（ASCII、可被 .cu 包含）。
 *
 * 提供 grid/block 计算以及带错误检查的 launch 宏。保持最小依赖。
 */

#pragma once

#include <cuda_runtime.h>

#include "CUDA/Core/Check.cuh"

namespace bee::cuda
{

// 向上取整：适合一维 grid-stride / 一 thread 一元素的 kernel
[[nodiscard]] __host__ __device__ inline unsigned int ceil_div_u(unsigned int a, unsigned int b) noexcept
{
    return (a + b - 1u) / b;
}

[[nodiscard]] __host__ __device__ inline unsigned long long ceil_div_u64(unsigned long long a, unsigned long long b) noexcept
{
    return (a + b - 1ull) / b;
}

// 默认 block size：256（与 plan §6.1 elementwise 连续快路径默认一致）。
inline constexpr int kDefaultBlockSize = 256;

// 根据元素总数与 block size 计算 grid，截断到 CUDA 最大 X 维（2^31-1）。
[[nodiscard]] inline unsigned int compute_grid_1d(unsigned long long n, unsigned int block = kDefaultBlockSize) noexcept
{
    const unsigned long long g = ceil_div_u64(n, static_cast<unsigned long long>(block));
    constexpr unsigned long long kMaxGridX = 2147483647ull; // 2^31 - 1
    return static_cast<unsigned int>(g > kMaxGridX ? kMaxGridX : g);
}

} // namespace bee::cuda

// 启动 kernel 表达式，形如 kernel<<<grid, block, smem, stream>>>(args...)。
// 变参宏：允许 <<<...>>> 与参数列表中包含逗号。
// 启动/上次错误若失败则 return int（与 BEE_CUDA_RET_ON_ERR 的 int 约定一致）。
#define BEE_CUDA_LAUNCH(...)                                               \
    do {                                                                   \
        __VA_ARGS__;                                                       \
        const cudaError_t _bee_cu_launch_err = cudaGetLastError();         \
        if (_bee_cu_launch_err != cudaSuccess) {                           \
            return static_cast<int>(_bee_cu_launch_err);                   \
        }                                                                  \
    } while (0)
