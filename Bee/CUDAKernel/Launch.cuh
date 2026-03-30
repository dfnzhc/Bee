/**
 * @File Launch.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cuda_runtime.h>
#include <cstdint>
#include <cuda_fp16.h>
#include <type_traits>

namespace bee::cuda
{

#if defined(BEE_HAS_WGMMA) || defined(BEE_HAS_CUTLASS)
static constexpr bool kBuildHasAdvancedPath = true;
#else
static constexpr bool kBuildHasAdvancedPath = false;
#endif

template <typename T>
struct AccumType
{
    using type = T;
};

template <>
struct AccumType<half>
{
    using type = float;
};

template <typename T>
using AccumType_t = AccumType<T>::type;

template <typename T>
__device__ __forceinline__ AccumType_t<T> to_accum(T v)
{
    if constexpr (std::is_same_v<T, half>) {
        return __half2float(v);
    } else {
        return static_cast<AccumType_t<T>>(v);
    }
}

template <typename T>
__device__ __forceinline__ T from_accum(AccumType_t<T> v)
{
    if constexpr (std::is_same_v<T, half>) {
        return __float2half_rn(v);
    } else {
        return static_cast<T>(v);
    }
}

/**
 * @brief Kernel 启动配置。
 *
 * 统一封装 grid/block/dynamic shared memory，便于上层策略函数返回并复用。
 */
struct Config
{
    dim3 grid{1, 1, 1};
    dim3 block{1, 1, 1};
    std::size_t dynamic_shared_bytes{0};
};

/**
 * @brief 向上整除（int 版本）。
 */
constexpr int ceil_div(int n, int d)
{
    return (n + d - 1) / d;
}

/**
 * @brief 向上整除（int64 版本）。
 */
constexpr std::int64_t ceil_div(std::int64_t n, std::int64_t d)
{
    return (n + d - 1) / d;
}

/**
 * @brief 构造一维并行配置。
 *
 * 典型用于 elementwise / reduce 前后处理等一维映射场景。
 */
inline Config make_1d(std::int64_t n, int block_size, std::size_t dynamic_shared_bytes = 0)
{
    Config cfg;
    cfg.block                = dim3(static_cast<unsigned>(block_size), 1, 1);
    cfg.grid                 = dim3(static_cast<unsigned>(ceil_div(n, static_cast<std::int64_t>(block_size))), 1, 1);
    cfg.dynamic_shared_bytes = dynamic_shared_bytes;
    return cfg;
}

template <typename Kernel>
/**
 * @brief 查询 CUDA occupancy 建议 block size。
 */
cudaError_t max_potential_block_size(Kernel kernel, int* min_grid_size, int* block_size, std::size_t dynamic_smem_bytes = 0)
{
    return cudaOccupancyMaxPotentialBlockSize(min_grid_size, block_size, kernel, static_cast<std::size_t>(dynamic_smem_bytes), 0);
}

template <typename Kernel>
/**
 * @brief 使用 occupancy 建议自动构造一维配置。
 *
 * 当查询失败时自动回退到 fallback_block，避免调用方额外兜底。
 */
Config make_occupancy_config(Kernel kernel, std::int64_t n, std::size_t dynamic_smem_bytes = 0, int fallback_block = 256)
{
    int min_grid_size     = 0;
    int block_size        = 0;
    const cudaError_t err = max_potential_block_size(kernel, &min_grid_size, &block_size, dynamic_smem_bytes);

    if (err != cudaSuccess || block_size <= 0) {
        return make_1d(n, fallback_block, dynamic_smem_bytes);
    }

    return make_1d(n, block_size, dynamic_smem_bytes);
}

inline bool check_sm_level(int smMajor)
{
    int dev = 0;
    if (cudaGetDevice(&dev) != cudaSuccess) {
        return false;
    }

    cudaDeviceProp prop{};
    if (cudaGetDeviceProperties(&prop, dev) != cudaSuccess) {
        return false;
    }
    return prop.major >= smMajor;
}

} // namespace bee::cuda
