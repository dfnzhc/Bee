/**
 * @File Ops/ElementWise.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Element-wise binary / unary kernels and integer-returning dispatch.
 *
 * ASCII-only TU compiled by nvcc. Kernels assume contiguous inputs/outputs
 * with identical shapes; broadcasting / stride handling stays on the host
 * side (Tensor layer performs contiguous() before entering these paths).
 *
 * Default block size = 256, one thread per element. Host dispatch reads
 * the per-thread default stream and synchronizes before returning, so the
 * API is observationally synchronous (plan-cuda section 5).
 */

#include "CUDA/Ops/OpsBridge.hpp"
#include "CUDA/Core/Check.cuh"
#include "CUDA/Core/Launch.cuh"

#include <cuda_runtime.h>

#include <cmath>
#include <cstdint>

namespace
{

// ScalarType mirror of bee::cuda::ScalarType.
constexpr int kDtBool = 0;
constexpr int kDtU8   = 1;
constexpr int kDtI32  = 2;
constexpr int kDtI64  = 3;
constexpr int kDtF32  = 4;
constexpr int kDtF64  = 5;

// BinaryOp values.
constexpr int kBinAdd = 0;
constexpr int kBinSub = 1;
constexpr int kBinMul = 2;
constexpr int kBinDiv = 3;

// UnaryOp values.
constexpr int kUnNeg  = 0;
constexpr int kUnAbs  = 1;
constexpr int kUnSqrt = 2;
constexpr int kUnExp  = 3;
constexpr int kUnLog  = 4;

template <typename T, int OP>
__global__ void binary_kernel(const T* __restrict__ a,
                              const T* __restrict__ b,
                              T* __restrict__ out,
                              std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n) return;
    const T av = a[i];
    const T bv = b[i];
    if constexpr (OP == kBinAdd) out[i] = av + bv;
    else if constexpr (OP == kBinSub) out[i] = av - bv;
    else if constexpr (OP == kBinMul) out[i] = av * bv;
    else /* Div */ out[i] = av / bv;
}

template <typename T>
__device__ inline T abs_dev(T v)
{
    if constexpr (std::is_floating_point_v<T>) return fabs(static_cast<double>(v));
    else if constexpr (std::is_unsigned_v<T>) return v;
    else return v < T(0) ? T(-v) : v;
}

template <typename T, int OP>
__global__ void unary_kernel(const T* __restrict__ a, T* __restrict__ out, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n) return;
    const T av = a[i];
    if constexpr (OP == kUnNeg)  out[i] = static_cast<T>(-static_cast<double>(av));
    else if constexpr (OP == kUnAbs) out[i] = abs_dev<T>(av);
    else if constexpr (OP == kUnSqrt) out[i] = static_cast<T>(::sqrt(static_cast<double>(av)));
    else if constexpr (OP == kUnExp)  out[i] = static_cast<T>(::exp(static_cast<double>(av)));
    else /* Log */ out[i] = static_cast<T>(::log(static_cast<double>(av)));
}

// Specialization: float path uses 32-bit math intrinsics.
template <int OP>
__global__ void unary_kernel_f32(const float* __restrict__ a, float* __restrict__ out, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n) return;
    const float av = a[i];
    if constexpr (OP == kUnNeg)  out[i] = -av;
    else if constexpr (OP == kUnAbs) out[i] = fabsf(av);
    else if constexpr (OP == kUnSqrt) out[i] = sqrtf(av);
    else if constexpr (OP == kUnExp)  out[i] = expf(av);
    else /* Log */ out[i] = logf(av);
}

template <typename T, int OP>
inline int launch_binary(const void* a, const void* b, void* out, std::size_t n, cudaStream_t stream)
{
    if (n == 0) return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    binary_kernel<T, OP><<<grid, block, 0, stream>>>(
        static_cast<const T*>(a), static_cast<const T*>(b), static_cast<T*>(out), n);
    return static_cast<int>(cudaGetLastError());
}

template <typename T, int OP>
inline int launch_unary(const void* a, void* out, std::size_t n, cudaStream_t stream)
{
    if (n == 0) return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    unary_kernel<T, OP><<<grid, block, 0, stream>>>(
        static_cast<const T*>(a), static_cast<T*>(out), n);
    return static_cast<int>(cudaGetLastError());
}

template <int OP>
inline int launch_unary_f32(const void* a, void* out, std::size_t n, cudaStream_t stream)
{
    if (n == 0) return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(n, block);
    unary_kernel_f32<OP><<<grid, block, 0, stream>>>(
        static_cast<const float*>(a), static_cast<float*>(out), n);
    return static_cast<int>(cudaGetLastError());
}

#define DISPATCH_BINARY_OP(OP_ID)                                                      \
    do {                                                                               \
        switch (dt) {                                                                  \
        case kDtU8:  err = launch_binary<std::uint8_t, OP_ID>(a, b, out, n, stream); break; \
        case kDtI32: err = launch_binary<std::int32_t, OP_ID>(a, b, out, n, stream); break; \
        case kDtI64: err = launch_binary<std::int64_t, OP_ID>(a, b, out, n, stream); break; \
        case kDtF32: err = launch_binary<float,        OP_ID>(a, b, out, n, stream); break; \
        case kDtF64: err = launch_binary<double,       OP_ID>(a, b, out, n, stream); break; \
        default: return static_cast<int>(cudaErrorInvalidValue);                       \
        }                                                                              \
    } while (0)

} // anonymous namespace

namespace bee::cuda::detail
{

int ops_binary(int op, int dt, const void* a, const void* b, void* out, std::size_t n) noexcept
{
    cudaStream_t stream = cudaStreamPerThread;
    int err = 0;
    switch (op) {
    case kBinAdd: DISPATCH_BINARY_OP(kBinAdd); break;
    case kBinSub: DISPATCH_BINARY_OP(kBinSub); break;
    case kBinMul: DISPATCH_BINARY_OP(kBinMul); break;
    case kBinDiv: DISPATCH_BINARY_OP(kBinDiv); break;
    default: return static_cast<int>(cudaErrorInvalidValue);
    }
    if (err != 0) return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

int ops_unary(int op, int dt, const void* a, void* out, std::size_t n) noexcept
{
    cudaStream_t stream = cudaStreamPerThread;
    int err = 0;

    // Float paths use dedicated f32 kernels for precision.
    if (dt == kDtF32) {
        switch (op) {
        case kUnNeg:  err = launch_unary_f32<kUnNeg >(a, out, n, stream); break;
        case kUnAbs:  err = launch_unary_f32<kUnAbs >(a, out, n, stream); break;
        case kUnSqrt: err = launch_unary_f32<kUnSqrt>(a, out, n, stream); break;
        case kUnExp:  err = launch_unary_f32<kUnExp >(a, out, n, stream); break;
        case kUnLog:  err = launch_unary_f32<kUnLog >(a, out, n, stream); break;
        default: return static_cast<int>(cudaErrorInvalidValue);
        }
    } else if (dt == kDtF64) {
        switch (op) {
        case kUnNeg:  err = launch_unary<double, kUnNeg >(a, out, n, stream); break;
        case kUnAbs:  err = launch_unary<double, kUnAbs >(a, out, n, stream); break;
        case kUnSqrt: err = launch_unary<double, kUnSqrt>(a, out, n, stream); break;
        case kUnExp:  err = launch_unary<double, kUnExp >(a, out, n, stream); break;
        case kUnLog:  err = launch_unary<double, kUnLog >(a, out, n, stream); break;
        default: return static_cast<int>(cudaErrorInvalidValue);
        }
    } else if (op == kUnNeg || op == kUnAbs) {
        // Integer neg/abs paths.
        switch (dt) {
        case kDtI32: err = (op == kUnNeg) ? launch_unary<std::int32_t, kUnNeg>(a, out, n, stream)
                                          : launch_unary<std::int32_t, kUnAbs>(a, out, n, stream); break;
        case kDtI64: err = (op == kUnNeg) ? launch_unary<std::int64_t, kUnNeg>(a, out, n, stream)
                                          : launch_unary<std::int64_t, kUnAbs>(a, out, n, stream); break;
        default: return static_cast<int>(cudaErrorInvalidValue);
        }
    } else {
        return static_cast<int>(cudaErrorInvalidValue);
    }

    if (err != 0) return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail
