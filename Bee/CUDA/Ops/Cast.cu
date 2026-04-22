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

template <typename Src, typename Dst>
__global__ void cast_kernel(const Src* __restrict__ src, Dst* __restrict__ dst, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n) return;
    dst[i] = static_cast<Dst>(src[i]);
}

// Specialization for Dst == bool: normalize to 0/1.
template <typename Src>
__global__ void cast_to_bool_kernel(const Src* __restrict__ src, std::uint8_t* __restrict__ dst, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n) return;
    dst[i] = (src[i] != Src(0)) ? std::uint8_t(1) : std::uint8_t(0);
}

template <typename Src, typename Dst>
inline int launch_cast(const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    if (n == 0) return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    cast_kernel<Src, Dst><<<grid, block, 0, stream>>>(
        static_cast<const Src*>(s), static_cast<Dst*>(d), n);
    return static_cast<int>(cudaGetLastError());
}

template <typename Src>
inline int launch_cast_to_bool(const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    if (n == 0) return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    cast_to_bool_kernel<Src><<<grid, block, 0, stream>>>(
        static_cast<const Src*>(s), static_cast<std::uint8_t*>(d), n);
    return static_cast<int>(cudaGetLastError());
}

template <typename Src>
inline int dispatch_dst(int dst_dt, const void* s, void* d, std::size_t n, cudaStream_t stream)
{
    switch (dst_dt) {
    case kDtBool: return launch_cast_to_bool<Src>(s, d, n, stream);
    case kDtU8:   return launch_cast<Src, std::uint8_t>(s, d, n, stream);
    case kDtI32:  return launch_cast<Src, std::int32_t>(s, d, n, stream);
    case kDtI64:  return launch_cast<Src, std::int64_t>(s, d, n, stream);
    case kDtF32:  return launch_cast<Src, float       >(s, d, n, stream);
    case kDtF64:  return launch_cast<Src, double      >(s, d, n, stream);
    default: return static_cast<int>(cudaErrorInvalidValue);
    }
}

} // namespace

namespace bee::cuda::detail
{

int ops_cast(int src_dt, const void* src, int dst_dt, void* dst, std::size_t n) noexcept
{
    cudaStream_t stream = cudaStreamPerThread;
    int err = 0;

    // Bool / U8 share the uint8 underlying representation: for Bool source we
    // treat input as already-normalized 0/1 (produced by Tensor layer), so a
    // plain numeric cast is correct.
    switch (src_dt) {
    case kDtBool: err = dispatch_dst<std::uint8_t>(dst_dt, src, dst, n, stream); break;
    case kDtU8:   err = dispatch_dst<std::uint8_t>(dst_dt, src, dst, n, stream); break;
    case kDtI32:  err = dispatch_dst<std::int32_t>(dst_dt, src, dst, n, stream); break;
    case kDtI64:  err = dispatch_dst<std::int64_t>(dst_dt, src, dst, n, stream); break;
    case kDtF32:  err = dispatch_dst<float       >(dst_dt, src, dst, n, stream); break;
    case kDtF64:  err = dispatch_dst<double      >(dst_dt, src, dst, n, stream); break;
    default: return static_cast<int>(cudaErrorInvalidValue);
    }

    if (err != 0) return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail
