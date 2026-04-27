/**
 * @File Ops/Cast.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief dtype conversion kernel with (src_dt, dst_dt) dispatch.
 *
 * ASCII-only TU. Assumes contiguous device buffers of equal element count.
 * Bool values are normalized to 0/1 when used as destination.
 */

#include "CUDA/Ops/OpsBridge.hpp"
#include "CUDA/Core/Check.cuh"
#include "CUDA/Core/Launch.cuh"

#include <cuda_bf16.h>
#include <cuda_fp16.h>
#include <cuda_runtime.h>

#include <cstdint>

namespace
{

constexpr int kDtBool = 0;
constexpr int kDtU8   = 1;
constexpr int kDtI32  = 2;
constexpr int kDtI64  = 3;
constexpr int kDtF32  = 4;
constexpr int kDtF64  = 5;
constexpr int kDtI8   = 6;
constexpr int kDtF16  = 7;
constexpr int kDtBF16 = 8;

template <typename Src, typename Dst>
__global__ void cast_kernel(const Src* __restrict__ src, Dst* __restrict__ dst, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    dst[i] = static_cast<Dst>(src[i]);
}

// Specialization for Dst == bool: normalize to 0/1.
template <typename Src>
__global__ void cast_to_bool_kernel(const Src* __restrict__ src, std::uint8_t* __restrict__ dst, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    dst[i] = (src[i] != Src(0)) ? std::uint8_t(1) : std::uint8_t(0);
}

__global__ void f32_to_f16_kernel(const float* __restrict__ src, __half* __restrict__ dst, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    dst[i] = __float2half(src[i]);
}

__global__ void f16_to_f32_kernel(const __half* __restrict__ src, float* __restrict__ dst, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    dst[i] = __half2float(src[i]);
}

__global__ void f32_to_bf16_kernel(const float* __restrict__ src, __nv_bfloat16* __restrict__ dst, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    dst[i] = __float2bfloat16(src[i]);
}

__global__ void bf16_to_f32_kernel(const __nv_bfloat16* __restrict__ src, float* __restrict__ dst, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    dst[i] = __bfloat162float(src[i]);
}

template <typename Src, typename Dst>
inline int launch_cast(const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    cast_kernel<Src, Dst><<<grid, block, 0, stream>>>(static_cast<const Src*>(s), static_cast<Dst*>(d), n);
    return static_cast<int>(cudaGetLastError());
}

template <typename Src>
inline int launch_cast_to_bool(const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    cast_to_bool_kernel<Src><<<grid, block, 0, stream>>>(static_cast<const Src*>(s), static_cast<std::uint8_t*>(d), n);
    return static_cast<int>(cudaGetLastError());
}

inline int launch_f32_to_f16(const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    f32_to_f16_kernel<<<grid, block, 0, stream>>>(static_cast<const float*>(s), static_cast<__half*>(d), n);
    return static_cast<int>(cudaGetLastError());
}

inline int launch_f16_to_f32(const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    f16_to_f32_kernel<<<grid, block, 0, stream>>>(static_cast<const __half*>(s), static_cast<float*>(d), n);
    return static_cast<int>(cudaGetLastError());
}

inline int launch_f32_to_bf16(const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    f32_to_bf16_kernel<<<grid, block, 0, stream>>>(static_cast<const float*>(s), static_cast<__nv_bfloat16*>(d), n);
    return static_cast<int>(cudaGetLastError());
}

inline int launch_bf16_to_f32(const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    bf16_to_f32_kernel<<<grid, block, 0, stream>>>(static_cast<const __nv_bfloat16*>(s), static_cast<float*>(d), n);
    return static_cast<int>(cudaGetLastError());
}

template <typename Src>
inline int dispatch_dst(int dst_dt, const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    switch (dst_dt) {
    case kDtBool: return launch_cast_to_bool<Src>(s, d, n, stream);
    case kDtU8: return launch_cast<Src, std::uint8_t>(s, d, n, stream);
    case kDtI32: return launch_cast<Src, std::int32_t>(s, d, n, stream);
    case kDtI64: return launch_cast<Src, std::int64_t>(s, d, n, stream);
    case kDtF32: return launch_cast<Src, float>(s, d, n, stream);
    case kDtF64: return launch_cast<Src, double>(s, d, n, stream);
    case kDtI8: return launch_cast<Src, std::int8_t>(s, d, n, stream);
    default: return static_cast<int>(cudaErrorInvalidValue);
    }
}

} // namespace

namespace bee::cuda::detail
{

int ops_cast(int src_dt, const void* src, int dst_dt, void* dst, std::size_t n) noexcept
{
    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    if (src_dt == kDtF32 && dst_dt == kDtF16)
        err = launch_f32_to_f16(src, dst, n, stream);
    else if (src_dt == kDtF16 && dst_dt == kDtF32)
        err = launch_f16_to_f32(src, dst, n, stream);
    else if (src_dt == kDtF32 && dst_dt == kDtBF16)
        err = launch_f32_to_bf16(src, dst, n, stream);
    else if (src_dt == kDtBF16 && dst_dt == kDtF32)
        err = launch_bf16_to_f32(src, dst, n, stream);
    else if (src_dt == kDtF16 || src_dt == kDtBF16 || dst_dt == kDtF16 || dst_dt == kDtBF16)
        return static_cast<int>(cudaErrorInvalidValue);
    else
    {
        switch (src_dt) {
        case kDtBool: err = dispatch_dst<std::uint8_t>(dst_dt, src, dst, n, stream); break;
        case kDtU8: err = dispatch_dst<std::uint8_t>(dst_dt, src, dst, n, stream); break;
        case kDtI32: err = dispatch_dst<std::int32_t>(dst_dt, src, dst, n, stream); break;
        case kDtI64: err = dispatch_dst<std::int64_t>(dst_dt, src, dst, n, stream); break;
        case kDtF32: err = dispatch_dst<float>(dst_dt, src, dst, n, stream); break;
        case kDtF64: err = dispatch_dst<double>(dst_dt, src, dst, n, stream); break;
        case kDtI8: err = dispatch_dst<std::int8_t>(dst_dt, src, dst, n, stream); break;
        default: return static_cast<int>(cudaErrorInvalidValue);
        }
    }

    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail
