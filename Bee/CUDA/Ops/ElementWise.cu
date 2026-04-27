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
constexpr int kUnRelu    = 5;
constexpr int kUnSigmoid = 6;

// ─── 向量化 traits：每个 dtype 选择最合适的 128-bit 载入向量 ──────────────────
// clang-format off
template <typename T> struct VecTraits;

template <> struct VecTraits<std::uint8_t>  { using vec_t = uint4;    static constexpr int kVecN = 16; };
template <> struct VecTraits<std::int32_t>  { using vec_t = int4;     static constexpr int kVecN = 4;  };
template <> struct VecTraits<float>         { using vec_t = float4;   static constexpr int kVecN = 4;  };
template <> struct VecTraits<std::int64_t>  { using vec_t = longlong2;static constexpr int kVecN = 2;  };
template <> struct VecTraits<double>        { using vec_t = double2;  static constexpr int kVecN = 2;  };
// clang-format on

template <typename T, typename VT>
__device__ __forceinline__ void load_vec(const T* p, T (&out)[VecTraits<T>::kVecN])
{
    const VT v = *reinterpret_cast<const VT*>(p);
    BEE_UNROLL
    for (int k = 0; k < VecTraits<T>::kVecN; ++k)
        out[k] = reinterpret_cast<const T*>(&v)[k];
}

template <typename T, typename VT>
__device__ __forceinline__ void store_vec(T* p, const T (&in)[VecTraits<T>::kVecN])
{
    VT v;
    BEE_UNROLL
    for (int k = 0; k < VecTraits<T>::kVecN; ++k)
        reinterpret_cast<T*>(&v)[k] = in[k];
    *reinterpret_cast<VT*>(p) = v;
}

template <typename T, int OP>
__device__ __forceinline__ T apply_bin(T av, T bv)
{
    if constexpr (OP == kBinAdd)
        return av + bv;
    else if constexpr (OP == kBinSub)
        return av - bv;
    else if constexpr (OP == kBinMul)
        return av * bv;
    else
        return av / bv;
}

// Grid-stride 向量化 binary kernel：每线程处理 VEC_N 元素（128-bit），
// 尾部 (n % VEC_N) 由尾核处理。
template <typename T, int OP>
__global__ void binary_kernel_vec(const T* __restrict__ a, const T* __restrict__ b, T* __restrict__ out, std::size_t n_vec)
{
    using VT                 = typename VecTraits<T>::vec_t;
    constexpr int     N      = VecTraits<T>::kVecN;
    const std::size_t tid    = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    const std::size_t stride = static_cast<std::size_t>(gridDim.x) * blockDim.x;
    for (std::size_t v = tid; v < n_vec; v += stride) {
        T a_buf[N], b_buf[N], o_buf[N];
        load_vec<T, VT>(a + v * N, a_buf);
        load_vec<T, VT>(b + v * N, b_buf);
        BEE_UNROLL
        for (int k = 0; k < N; ++k)
            o_buf[k] = apply_bin<T, OP>(a_buf[k], b_buf[k]);
        store_vec<T, VT>(out + v * N, o_buf);
    }
}

template <typename T, int OP>
__global__ void binary_kernel_tail(const T* __restrict__ a, const T* __restrict__ b, T* __restrict__ out, std::size_t start, std::size_t n)
{
    const std::size_t i = start + static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    out[i] = apply_bin<T, OP>(a[i], b[i]);
}

template <typename T, int OP>
__global__ void binary_kernel(const T* __restrict__ a, const T* __restrict__ b, T* __restrict__ out, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    out[i] = apply_bin<T, OP>(a[i], b[i]);
}

template <typename T>
__device__ inline T abs_dev(T v)
{
    if constexpr (std::is_floating_point_v<T>)
        return fabs(static_cast<double>(v));
    else if constexpr (std::is_unsigned_v<T>)
        return v;
    else
        return v < T(0) ? T(-v) : v;
}

template <typename T, int OP>
__device__ __forceinline__ T apply_un(T v)
{
    if constexpr (OP == kUnNeg) {
        if constexpr (std::is_unsigned_v<T>)
            return static_cast<T>(-static_cast<std::int64_t>(v));
        else
            return static_cast<T>(-v);
    } else if constexpr (OP == kUnAbs) {
        if constexpr (std::is_unsigned_v<T>)
            return v;
        else if constexpr (std::is_floating_point_v<T>) {
            if constexpr (std::is_same_v<T, float>)
                return fabsf(v);
            else
                return fabs(v);
        } else
            return v < T(0) ? T(-v) : v;
    } else if constexpr (OP == kUnRelu) {
        return v < T(0) ? T(0) : v;
    }
    return v;
}

// 向量化 unary kernel 只覆盖 neg/abs/relu 等 memory-bound op；
// sqrt/exp/log/sigmoid 等 compute-bound op 走标量路径，避免无收益的向量打包。
template <typename T, int OP>
__global__ void unary_kernel_vec(const T* __restrict__ a, T* __restrict__ out, std::size_t n_vec)
{
    using VT                 = typename VecTraits<T>::vec_t;
    constexpr int     N      = VecTraits<T>::kVecN;
    const std::size_t tid    = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    const std::size_t stride = static_cast<std::size_t>(gridDim.x) * blockDim.x;
    for (std::size_t v = tid; v < n_vec; v += stride) {
        T a_buf[N], o_buf[N];
        load_vec<T, VT>(a + v * N, a_buf);
        BEE_UNROLL
        for (int k = 0; k < N; ++k)
            o_buf[k] = apply_un<T, OP>(a_buf[k]);
        store_vec<T, VT>(out + v * N, o_buf);
    }
}

template <typename T, int OP>
__global__ void unary_kernel_tail(const T* __restrict__ a, T* __restrict__ out, std::size_t start, std::size_t n)
{
    const std::size_t i = start + static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    out[i] = apply_un<T, OP>(a[i]);
}

template <typename T, int OP>
__global__ void unary_kernel(const T* __restrict__ a, T* __restrict__ out, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    const T av = a[i];
    if constexpr (OP == kUnNeg)
        out[i] = static_cast<T>(-static_cast<double>(av));
    else if constexpr (OP == kUnAbs)
        out[i] = abs_dev<T>(av);
    else if constexpr (OP == kUnSqrt)
        out[i] = static_cast<T>(::sqrt(static_cast<double>(av)));
    else if constexpr (OP == kUnExp)
        out[i] = static_cast<T>(::exp(static_cast<double>(av)));
    else if constexpr (OP == kUnLog)
        out[i] = static_cast<T>(::log(static_cast<double>(av)));
    else if constexpr (OP == kUnRelu)
        out[i] = av < T(0) ? T(0) : av;
    else { /* Sigmoid */
        const double x = static_cast<double>(av);
        if (x >= 0.0)
            out[i] = static_cast<T>(1.0 / (1.0 + ::exp(-x)));
        else {
            const double e = ::exp(x);
            out[i]         = static_cast<T>(e / (1.0 + e));
        }
    }
}

// Specialization: float path uses 32-bit math intrinsics.
template <int OP>
__global__ void unary_kernel_f32(const float* __restrict__ a, float* __restrict__ out, std::size_t n)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n)
        return;
    const float av = a[i];
    if constexpr (OP == kUnNeg)
        out[i] = -av;
    else if constexpr (OP == kUnAbs)
        out[i] = fabsf(av);
    else if constexpr (OP == kUnSqrt)
        out[i] = sqrtf(av);
    else if constexpr (OP == kUnExp)
        out[i] = expf(av);
    else if constexpr (OP == kUnLog)
        out[i] = logf(av);
    else if constexpr (OP == kUnRelu)
        out[i] = av < 0.0f ? 0.0f : av;
    else { /* Sigmoid */
        if (av >= 0.0f)
            out[i] = 1.0f / (1.0f + expf(-av));
        else {
            const float e = expf(av);
            out[i]        = e / (1.0f + e);
        }
    }
}

template <typename T, int OP>
inline int launch_binary(const void* a, const void* b, void* out, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return 0;
    using VT                = typename VecTraits<T>::vec_t;
    constexpr int     N     = VecTraits<T>::kVecN;
    const std::size_t n_vec = n / N;
    const std::size_t tail  = n_vec * N;

    const unsigned int block = bee::cuda::kDefaultBlockSize;
    if (n_vec > 0) {
        // 限制 grid 到 SM 饱和规模，利用 grid-stride 处理大向量
        const unsigned int grid = static_cast<unsigned int>(n_vec < 512 ? n_vec : 512);
        binary_kernel_vec<T, OP><<<grid, block, 0, stream>>>(static_cast<const T*>(a), static_cast<const T*>(b), static_cast<T*>(out), n_vec);
    }
    if (tail < n) {
        const std::size_t  rem       = n - tail;
        const unsigned int grid_tail = bee::cuda::compute_grid_1d(rem, block);
        binary_kernel_tail<T, OP><<<grid_tail, block, 0, stream>>>(static_cast<const T*>(a), static_cast<const T*>(b), static_cast<T*>(out), tail, n);
    }
    return static_cast<int>(cudaGetLastError());
}

template <typename T, int OP>
inline int launch_unary(const void* a, void* out, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    // Neg/Abs/Relu 走向量化路径（memory-bound，128-bit 读写收益最高）
    if constexpr (OP == kUnNeg || OP == kUnAbs || OP == kUnRelu) {
        constexpr int     N     = VecTraits<T>::kVecN;
        const std::size_t n_vec = n / N;
        const std::size_t tail  = n_vec * N;
        if (n_vec > 0) {
            const unsigned int grid = static_cast<unsigned int>(n_vec < 512 ? n_vec : 512);
            unary_kernel_vec<T, OP><<<grid, block, 0, stream>>>(static_cast<const T*>(a), static_cast<T*>(out), n_vec);
        }
        if (tail < n) {
            const std::size_t  rem       = n - tail;
            const unsigned int grid_tail = bee::cuda::compute_grid_1d(rem, block);
            unary_kernel_tail<T, OP><<<grid_tail, block, 0, stream>>>(static_cast<const T*>(a), static_cast<T*>(out), tail, n);
        }
    } else {
        const unsigned int grid = bee::cuda::compute_grid_1d(n, block);
        unary_kernel<T, OP><<<grid, block, 0, stream>>>(static_cast<const T*>(a), static_cast<T*>(out), n);
    }
    return static_cast<int>(cudaGetLastError());
}

template <int OP>
inline int launch_unary_f32(const void* a, void* out, std::size_t n, cudaStream_t stream)
{
    if (n == 0)
        return 0;
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    if constexpr (OP == kUnNeg || OP == kUnAbs || OP == kUnRelu) {
        constexpr int     N     = VecTraits<float>::kVecN;
        const std::size_t n_vec = n / N;
        const std::size_t tail  = n_vec * N;
        if (n_vec > 0) {
            const unsigned int grid = static_cast<unsigned int>(n_vec < 512 ? n_vec : 512);
            unary_kernel_vec<float, OP><<<grid, block, 0, stream>>>(static_cast<const float*>(a), static_cast<float*>(out), n_vec);
        }
        if (tail < n) {
            const std::size_t  rem       = n - tail;
            const unsigned int grid_tail = bee::cuda::compute_grid_1d(rem, block);
            unary_kernel_tail<float, OP><<<grid_tail, block, 0, stream>>>(static_cast<const float*>(a), static_cast<float*>(out), tail, n);
        }
    } else {
        const unsigned int grid = bee::cuda::compute_grid_1d(n, block);
        unary_kernel_f32<OP><<<grid, block, 0, stream>>>(static_cast<const float*>(a), static_cast<float*>(out), n);
    }
    return static_cast<int>(cudaGetLastError());
}

#define DISPATCH_BINARY_OP(OP_ID)                                                           \
    do {                                                                                    \
        switch (dt) {                                                                       \
        case kDtU8: err = launch_binary<std::uint8_t, OP_ID>(a, b, out, n, stream); break;  \
        case kDtI32: err = launch_binary<std::int32_t, OP_ID>(a, b, out, n, stream); break; \
        case kDtI64: err = launch_binary<std::int64_t, OP_ID>(a, b, out, n, stream); break; \
        case kDtF32: err = launch_binary<float, OP_ID>(a, b, out, n, stream); break;        \
        case kDtF64: err = launch_binary<double, OP_ID>(a, b, out, n, stream); break;       \
        default: return static_cast<int>(cudaErrorInvalidValue);                            \
        }                                                                                   \
    } while (0)

} // anonymous namespace

namespace bee::cuda::detail
{

int ops_binary(int op, int dt, const void* a, const void* b, void* out, std::size_t n) noexcept
{
    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;
    switch (op) {
    case kBinAdd: DISPATCH_BINARY_OP(kBinAdd); break;
    case kBinSub: DISPATCH_BINARY_OP(kBinSub); break;
    case kBinMul: DISPATCH_BINARY_OP(kBinMul); break;
    case kBinDiv: DISPATCH_BINARY_OP(kBinDiv); break;
    default: return static_cast<int>(cudaErrorInvalidValue);
    }
    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

int ops_unary(int op, int dt, const void* a, void* out, std::size_t n) noexcept
{
    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    // Float paths use dedicated f32 kernels for precision.
    if (dt == kDtF32) {
        switch (op) {
        case kUnNeg: err = launch_unary_f32<kUnNeg>(a, out, n, stream); break;
        case kUnAbs: err = launch_unary_f32<kUnAbs>(a, out, n, stream); break;
        case kUnSqrt: err = launch_unary_f32<kUnSqrt>(a, out, n, stream); break;
        case kUnExp: err = launch_unary_f32<kUnExp>(a, out, n, stream); break;
        case kUnLog: err = launch_unary_f32<kUnLog>(a, out, n, stream); break;
        case kUnRelu: err = launch_unary_f32<kUnRelu>(a, out, n, stream); break;
        case kUnSigmoid: err = launch_unary_f32<kUnSigmoid>(a, out, n, stream); break;
        default: return static_cast<int>(cudaErrorInvalidValue);
        }
    } else if (dt == kDtF64) {
        switch (op) {
        case kUnNeg: err = launch_unary<double, kUnNeg>(a, out, n, stream); break;
        case kUnAbs: err = launch_unary<double, kUnAbs>(a, out, n, stream); break;
        case kUnSqrt: err = launch_unary<double, kUnSqrt>(a, out, n, stream); break;
        case kUnExp: err = launch_unary<double, kUnExp>(a, out, n, stream); break;
        case kUnLog: err = launch_unary<double, kUnLog>(a, out, n, stream); break;
        case kUnRelu: err = launch_unary<double, kUnRelu>(a, out, n, stream); break;
        case kUnSigmoid: err = launch_unary<double, kUnSigmoid>(a, out, n, stream); break;
        default: return static_cast<int>(cudaErrorInvalidValue);
        }
    } else if (op == kUnNeg || op == kUnAbs || op == kUnRelu) {
        // Integer neg/abs/relu paths.
        switch (dt) {
        case kDtI32:
            if (op == kUnNeg)
                err = launch_unary<std::int32_t, kUnNeg>(a, out, n, stream);
            else if (op == kUnAbs)
                err = launch_unary<std::int32_t, kUnAbs>(a, out, n, stream);
            else
                err = launch_unary<std::int32_t, kUnRelu>(a, out, n, stream);
            break;
        case kDtI64:
            if (op == kUnNeg)
                err = launch_unary<std::int64_t, kUnNeg>(a, out, n, stream);
            else if (op == kUnAbs)
                err = launch_unary<std::int64_t, kUnAbs>(a, out, n, stream);
            else
                err = launch_unary<std::int64_t, kUnRelu>(a, out, n, stream);
            break;
        default: return static_cast<int>(cudaErrorInvalidValue);
        }
    } else {
        return static_cast<int>(cudaErrorInvalidValue);
    }

    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail
