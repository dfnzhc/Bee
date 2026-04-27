/**
 * @File Ops/Softmax.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/27
 * @Brief F32/F64 原生 CUDA 稳定 softmax。
 */

#include "CUDA/Ops/OpsBridge.hpp"
#include "CUDA/Core/Launch.cuh"

#include <cuda_runtime.h>

#include <cfloat>

namespace
{

constexpr int kDtF32 = 4;
constexpr int kDtF64 = 5;

__device__ auto exp_value(float v) -> float
{
    return expf(v);
}

__device__ auto exp_value(double v) -> double
{
    return exp(v);
}

template <typename T>
__device__ auto lowest_value() -> T;

template <>
__device__ auto lowest_value<float>() -> float
{
    return -FLT_MAX;
}

template <>
__device__ auto lowest_value<double>() -> double
{
    return -DBL_MAX;
}

template <typename T>
__global__ void softmax_kernel(const T* __restrict__ src, T* __restrict__ dst, std::size_t outer, std::size_t axis, std::size_t inner)
{
    const std::size_t total = outer * inner;
    const std::size_t lane  = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (lane >= total)
        return;

    const std::size_t o    = lane / inner;
    const std::size_t i    = lane % inner;
    const std::size_t base = o * axis * inner + i;

    // 第一遍：沿 axis 找最大值，避免指数溢出。
    T maxv = lowest_value<T>();
    for (std::size_t a = 0; a < axis; ++a) {
        const T v = src[base + a * inner];
        maxv      = v > maxv ? v : maxv;
    }

    // 第二遍：写入 exp(x - max) 并累加分母。
    T sum = T(0);
    for (std::size_t a = 0; a < axis; ++a) {
        const std::size_t idx = base + a * inner;
        const T           v   = exp_value(src[idx] - maxv);
        dst[idx]              = v;
        sum                  += v;
    }

    // 第三遍：原位归一化。
    for (std::size_t a = 0; a < axis; ++a) {
        const std::size_t idx = base + a * inner;
        dst[idx]             /= sum;
    }
}

template <typename T>
auto launch_softmax(const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner, cudaStream_t stream) -> int
{
    const std::size_t total = outer * inner;
    if (total == 0 || axis == 0 || src == nullptr || dst == nullptr)
        return static_cast<int>(cudaErrorInvalidValue);

    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(total, block);
    softmax_kernel<T><<<grid, block, 0, stream>>>(static_cast<const T*>(src), static_cast<T*>(dst), outer, axis, inner);
    return static_cast<int>(cudaGetLastError());
}

} // 匿名命名空间

namespace bee::cuda::detail
{

int ops_softmax(int dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) noexcept
{
    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    switch (dt) {
    case kDtF32: err = launch_softmax<float>(src, dst, outer, axis, inner, stream); break;
    case kDtF64: err = launch_softmax<double>(src, dst, outer, axis, inner, stream); break;
    default: return static_cast<int>(cudaErrorInvalidValue);
    }

    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // 命名空间 bee::cuda::detail
