/**
 * @File Ops/Reduce.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Global / axis reduce (Sum/Min/Max/Prod) for I32/I64/F32/F64 (+U8 min/max).
 *
 * ASCII-only TU. Contiguous layout assumed by host dispatch. Mean is composed
 * at the Tensor layer (sum then scalar divide).
 */

#include "CUDA/Ops/OpsBridge.hpp"
#include "CUDA/Core/Check.cuh"
#include "CUDA/Core/Launch.cuh"
#include "CUDA/Core/Warp.cuh"
#include "CUDA/Core/Block.cuh"

#include <cuda_runtime.h>
#include <cooperative_groups.h>

#include <cfloat>
#include <climits>
#include <cstdint>
#include <limits>

namespace
{

constexpr int kDtBool = 0;
constexpr int kDtU8   = 1;
constexpr int kDtI32  = 2;
constexpr int kDtI64  = 3;
constexpr int kDtF32  = 4;
constexpr int kDtF64  = 5;

constexpr int kRdSum  = 0;
constexpr int kRdMin  = 1;
constexpr int kRdMax  = 2;
constexpr int kRdProd = 3;

template <typename T, int OP>
struct Reducer
{
    __device__ static T identity()
    {
        if constexpr (OP == kRdSum)
            return T(0);
        if constexpr (OP == kRdProd)
            return T(1);
        if constexpr (OP == kRdMin) {
            if constexpr (std::is_same_v<T, float>)
                return INFINITY;
            else if constexpr (std::is_same_v<T, double>)
                return (double)INFINITY;
            else if constexpr (std::is_same_v<T, std::uint8_t>)
                return T(0xFF);
            else if constexpr (std::is_same_v<T, std::int32_t>)
                return T(INT32_MAX);
            else if constexpr (std::is_same_v<T, std::int64_t>)
                return T(INT64_MAX);
            else
                return T();
        }
        if constexpr (OP == kRdMax) {
            if constexpr (std::is_same_v<T, float>)
                return -INFINITY;
            else if constexpr (std::is_same_v<T, double>)
                return -(double)INFINITY;
            else if constexpr (std::is_same_v<T, std::uint8_t>)
                return T(0);
            else if constexpr (std::is_same_v<T, std::int32_t>)
                return T(INT32_MIN);
            else if constexpr (std::is_same_v<T, std::int64_t>)
                return T(INT64_MIN);
            else
                return T();
        }
        return T{};
    }

    __device__ static T combine(T a, T b)
    {
        if constexpr (OP == kRdSum)
            return a + b;
        if constexpr (OP == kRdProd)
            return a * b;
        if constexpr (OP == kRdMin)
            return a < b ? a : b;
        if constexpr (OP == kRdMax)
            return a > b ? a : b;
        return a;
    }
};

template <int OP>
struct OpTrait;

template <>
struct OpTrait<kRdSum>
{
    using type = bee::cuda::WarpOpSum;
};

template <>
struct OpTrait<kRdProd>
{
    using type = bee::cuda::WarpOpProd;
};

template <>
struct OpTrait<kRdMin>
{
    using type = bee::cuda::WarpOpMin;
};

template <>
struct OpTrait<kRdMax>
{
    using type = bee::cuda::WarpOpMax;
};

// ── Global reduce ─────────────────────────────────────────────────────────
//
// Two-stage: (1) each block reduces its chunk to one scalar via
// BlockReduce (warp-shuffle based), (2) host launches the same kernel on
// the partial array until it collapses to 1.

constexpr int kReduceBlockSize = 256;

template <typename T, int OP>
__global__ void reduce_global_kernel(const T* __restrict__ src, T* __restrict__ partial, std::size_t n)
{
    using R     = Reducer<T, OP>;
    using Op    = typename OpTrait<OP>::type;
    using Block = bee::cuda::BlockReduce<T, kReduceBlockSize, Op>;

    __shared__ typename Block::TempStorage smem;

    const std::size_t tid    = threadIdx.x;
    const std::size_t stride = static_cast<std::size_t>(blockDim.x) * gridDim.x;

    // Grid-stride accumulate, unrolled by 4 to hide memory latency.
    T        acc = R::identity();
    std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + tid;
    const std::size_t stride4 = stride * 4;
    for (; i + stride4 <= n; i += stride4) {
        acc = R::combine(acc, src[i]);
        acc = R::combine(acc, src[i + stride]);
        acc = R::combine(acc, src[i + stride * 2]);
        acc = R::combine(acc, src[i + stride * 3]);
    }
    for (; i < n; i += stride) {
        acc = R::combine(acc, src[i]);
    }

    // Warp-shuffle block reduction.
    T out = Block(smem).ReduceWithIdentity(acc, R::identity(), Op{});
    if (tid == 0)
        partial[blockIdx.x] = out;
}

// ── Axis reduce ───────────────────────────────────────────────────────────
//
// Input treated as [outer, axis, inner] row-major. One thread per output
// element (o, i) -> dst[o * inner + i] = reduce_{k=0..axis} src[o*axis*inner + k*inner + i]
// This is simple and correct; optimization is future work.

template <typename T, int OP>
__global__ void reduce_axis_kernel(const T* __restrict__ src, T* __restrict__ dst, std::size_t outer, std::size_t axis, std::size_t inner)
{
    using R                 = Reducer<T, OP>;
    const std::size_t total = outer * inner;
    const std::size_t idx   = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (idx >= total)
        return;
    const std::size_t o    = idx / inner;
    const std::size_t i    = idx % inner;
    const T*          base = src + o * axis * inner + i;
    T                 acc  = R::identity();
    for (std::size_t k = 0; k < axis; ++k) {
        acc = R::combine(acc, base[k * inner]);
    }
    dst[idx] = acc;
}

// ── launcher helpers ──────────────────────────────────────────────────────

template <typename T>
__global__ void scale_fp_kernel(T* buf, T factor, std::size_t n)
{
    const std::size_t idx = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (idx < n)
        buf[idx] *= factor;
}

template <typename T, int OP>
int launch_global(const void* src, void* dst, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return static_cast<int>(cudaErrorInvalidValue);

    constexpr unsigned int block = static_cast<unsigned int>(kReduceBlockSize);
    unsigned int           grid  = static_cast<unsigned int>((n + block - 1) / block);
    if (grid > 1024u)
        grid = 1024u;

    // Stage 1: reduce to `grid` partials
    T*          partial = nullptr;
    cudaError_t err     = cudaMallocAsync(reinterpret_cast<void**>(&partial), grid * sizeof(T), stream);
    if (err != cudaSuccess)
        return static_cast<int>(err);

    reduce_global_kernel<T, OP><<<grid, block, 0, stream>>>(static_cast<const T*>(src), partial, n);
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        cudaFreeAsync(partial, stream);
        return static_cast<int>(err);
    }

    // Stage 2: reduce `grid` partials to a single scalar
    reduce_global_kernel<T, OP><<<1u, block, 0, stream>>>(partial, static_cast<T*>(dst), grid);
    err = cudaGetLastError();
    cudaFreeAsync(partial, stream);
    return static_cast<int>(err);
}

template <typename T, int OP>
int launch_axis(const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner, cudaStream_t stream)
{
    const std::size_t total = outer * inner;
    if (total == 0 || axis == 0)
        return static_cast<int>(cudaErrorInvalidValue);
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(total, block);
    reduce_axis_kernel<T, OP><<<grid, block, 0, stream>>>(static_cast<const T*>(src), static_cast<T*>(dst), outer, axis, inner);
    return static_cast<int>(cudaGetLastError());
}

#define DISPATCH_DT_ALL(CALL)                                \
    switch (dt) {                                            \
    case kDtU8: err = CALL(std::uint8_t); break;             \
    case kDtI32: err = CALL(std::int32_t); break;            \
    case kDtI64: err = CALL(std::int64_t); break;            \
    case kDtF32: err = CALL(float); break;                   \
    case kDtF64: err = CALL(double); break;                  \
    default: return static_cast<int>(cudaErrorInvalidValue); \
    }

#define DISPATCH_DT_NOU8(CALL)                               \
    switch (dt) {                                            \
    case kDtI32: err = CALL(std::int32_t); break;            \
    case kDtI64: err = CALL(std::int64_t); break;            \
    case kDtF32: err = CALL(float); break;                   \
    case kDtF64: err = CALL(double); break;                  \
    default: return static_cast<int>(cudaErrorInvalidValue); \
    }

} // namespace

namespace bee::cuda::detail
{

int ops_reduce_global(int op, int dt, const void* src, void* dst, std::size_t n) noexcept
{
    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    // U8 only allowed for Min/Max.
    if (op == kRdMin) {
#define CALL(T) launch_global<T, kRdMin>(src, dst, n, stream)
        DISPATCH_DT_ALL(CALL);
#undef CALL
    } else if (op == kRdMax) {
#define CALL(T) launch_global<T, kRdMax>(src, dst, n, stream)
        DISPATCH_DT_ALL(CALL);
#undef CALL
    } else if (op == kRdSum) {
#define CALL(T) launch_global<T, kRdSum>(src, dst, n, stream)
        DISPATCH_DT_NOU8(CALL);
#undef CALL
    } else if (op == kRdProd) {
#define CALL(T) launch_global<T, kRdProd>(src, dst, n, stream)
        DISPATCH_DT_NOU8(CALL);
#undef CALL
    } else {
        return static_cast<int>(cudaErrorInvalidValue);
    }

    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

int ops_reduce_axis(int op, int dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) noexcept
{
    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    if (op == kRdMin) {
#define CALL(T) launch_axis<T, kRdMin>(src, dst, outer, axis, inner, stream)
        DISPATCH_DT_ALL(CALL);
#undef CALL
    } else if (op == kRdMax) {
#define CALL(T) launch_axis<T, kRdMax>(src, dst, outer, axis, inner, stream)
        DISPATCH_DT_ALL(CALL);
#undef CALL
    } else if (op == kRdSum) {
#define CALL(T) launch_axis<T, kRdSum>(src, dst, outer, axis, inner, stream)
        DISPATCH_DT_NOU8(CALL);
#undef CALL
    } else if (op == kRdProd) {
#define CALL(T) launch_axis<T, kRdProd>(src, dst, outer, axis, inner, stream)
        DISPATCH_DT_NOU8(CALL);
#undef CALL
    } else {
        return static_cast<int>(cudaErrorInvalidValue);
    }

    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

int ops_scale_fp(int dt, void* buf, double factor, std::size_t n) noexcept
{
    if (n == 0)
        return 0;
    cudaStream_t       stream = cudaStreamPerThread;
    const unsigned int block  = bee::cuda::kDefaultBlockSize;
    const unsigned int grid   = bee::cuda::compute_grid_1d(n, block);
    if (dt == kDtF32) {
        scale_fp_kernel<float><<<grid, block, 0, stream>>>(static_cast<float*>(buf), static_cast<float>(factor), n);
    } else if (dt == kDtF64) {
        scale_fp_kernel<double><<<grid, block, 0, stream>>>(static_cast<double*>(buf), factor, n);
    } else {
        return static_cast<int>(cudaErrorInvalidValue);
    }
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess)
        return static_cast<int>(err);
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail
