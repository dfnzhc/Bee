/**
 * @File Ops/Matmul.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 通用 tiled shared-memory GEMM，实现 Bee::CUDA 的基础 matmul 路径。
 *
 * 本文件承担所有设备都可用的 baseline 行为：
 * - 支持 F32/F64/I32/I64；
 * - 正确处理非 TILE 整倍数的边界；
 * - 更高阶的 CUTLASS / Native(TMA) 路径放在独立 TU 中，由上层显式选择或自动回退。
 * - Task 5：ops_matmul_lowp 实现 F16/BF16 输入、F32 累加、F32 输出的低精度 GEMM。
 */

#include "CUDA/Ops/OpsBridge.hpp"
#include "CUDA/Core/Launch.cuh"

#include <cuda_bf16.h>
#include <cuda_fp16.h>
#include <cuda_runtime.h>
#include <cstdint>

namespace
{

constexpr int kDtI32  = 2;
constexpr int kDtI64  = 3;
constexpr int kDtF32  = 4;
constexpr int kDtF64  = 5;
constexpr int kDtF16  = 7;
constexpr int kDtBF16 = 8;

constexpr int TILE = 16;

template <typename T>
__global__ void matmul_tiled_kernel(const T* __restrict__ A, const T* __restrict__ B, T* __restrict__ C, std::size_t M, std::size_t K, std::size_t N)
{
    __shared__ T As[TILE][TILE];
    __shared__ T Bs[TILE][TILE];

    const std::size_t row = static_cast<std::size_t>(blockIdx.y) * TILE + threadIdx.y;
    const std::size_t col = static_cast<std::size_t>(blockIdx.x) * TILE + threadIdx.x;

    T                 acc   = T(0);
    const std::size_t tiles = (K + TILE - 1) / TILE;
    for (std::size_t t = 0; t < tiles; ++t) {
        const std::size_t a_col      = t * TILE + threadIdx.x;
        const std::size_t b_row      = t * TILE + threadIdx.y;
        As[threadIdx.y][threadIdx.x] = (row < M && a_col < K) ? A[row * K + a_col] : T(0);
        Bs[threadIdx.y][threadIdx.x] = (b_row < K && col < N) ? B[b_row * N + col] : T(0);
        __syncthreads();

        BEE_UNROLL
        for (int k = 0; k < TILE; ++k) {
            acc += As[threadIdx.y][k] * Bs[k][threadIdx.x];
        }
        __syncthreads();
    }

    if (row < M && col < N) {
        C[row * N + col] = acc;
    }
}

template <typename T>
int launch_matmul(const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N, cudaStream_t stream)
{
    dim3 block(TILE, TILE);
    dim3 grid(static_cast<unsigned int>((N + TILE - 1) / TILE), static_cast<unsigned int>((M + TILE - 1) / TILE));
    matmul_tiled_kernel<T><<<grid, block, 0, stream>>>(static_cast<const T*>(A), static_cast<const T*>(B), static_cast<T*>(C), M, K, N);
    return static_cast<int>(cudaGetLastError());
}

// ── 低精度 GEMM 辅助 ──────────────────────────────────────────────────────────

// 设备侧类型转换辅助：将低精度存储类型转为 float
template <typename T>
__device__ __forceinline__ float lowp_to_float(T v);

template <>
__device__ __forceinline__ float lowp_to_float<__half>(__half v)
{
    return __half2float(v);
}

template <>
__device__ __forceinline__ float lowp_to_float<__nv_bfloat16>(__nv_bfloat16 v)
{
    return __bfloat162float(v);
}

// 低精度矩阵乘核函数：读取 F16/BF16 输入，以 float 累加，写入 float 输出
// 每个线程负责一个输出元素，边界保护防止越界读写
template <typename LowP>
__global__ void matmul_lowp_kernel(
    const LowP* __restrict__ A,
    const LowP* __restrict__ B,
    float* __restrict__      C,
    std::size_t              M,
    std::size_t              K,
    std::size_t              N
)
{
    const std::size_t row = static_cast<std::size_t>(blockIdx.y) * blockDim.y + threadIdx.y;
    const std::size_t col = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (row >= M || col >= N)
        return;

    float acc = 0.0f;
    for (std::size_t k = 0; k < K; ++k) {
        acc += lowp_to_float(A[row * K + k]) * lowp_to_float(B[k * N + col]);
    }
    C[row * N + col] = acc;
}

template <typename LowP>
int launch_matmul_lowp(const void* A, const void* B, float* C, std::size_t M, std::size_t K, std::size_t N, cudaStream_t stream)
{
    dim3 block(TILE, TILE);
    dim3 grid(
        static_cast<unsigned int>((N + TILE - 1) / TILE),
        static_cast<unsigned int>((M + TILE - 1) / TILE)
    );
    matmul_lowp_kernel<LowP><<<grid, block, 0, stream>>>(
        static_cast<const LowP*>(A),
        static_cast<const LowP*>(B),
        C, M, K, N
    );
    return static_cast<int>(cudaGetLastError());
}

} // namespace

namespace bee::cuda::detail
{

#if BEE_HAS_CUTLASS
// 声明：实现在 MatmulCutlass.cu
int ops_matmul_f32_cutlass(const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N, cudaStream_t stream) noexcept;
#endif

// 声明：实现在 MatmulTmaWmma.cu。
// 该路径当前仅对 Blackwell Native 后端开放；其余设备返回明确错误或由上层阻止进入。
int ops_matmul_f32_tma_wmma(const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N, cudaStream_t stream) noexcept;

int ops_matmul_force_tma_wmma(int dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) noexcept
{
    if (M == 0 || N == 0)
        return 0;
    if (dt != kDtF32)
        return static_cast<int>(cudaErrorInvalidValue);
    cudaStream_t stream = cudaStreamPerThread;
    int          rc     = ops_matmul_f32_tma_wmma(A, B, C, M, K, N, stream);
    if (rc != 0)
        return rc;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

int ops_matmul(int dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) noexcept
{
    if (M == 0 || N == 0)
        return 0;
    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    // 自动路径优先尝试更高性能实现；若条件不满足，则稳定回退到 baseline kernel。
#if BEE_HAS_CUTLASS
    if (dt == kDtF32 && M >= 384 && N >= 384 && K >= 32) {
        int cu_err = ops_matmul_f32_cutlass(A, B, C, M, K, N, stream);
        if (cu_err == 0) {
            return static_cast<int>(cudaStreamSynchronize(stream));
        }
        // 非 4 对齐等情况：回退
    }
#endif

    switch (dt) {
    case kDtI32: err = launch_matmul<std::int32_t>(A, B, C, M, K, N, stream); break;
    case kDtI64: err = launch_matmul<std::int64_t>(A, B, C, M, K, N, stream); break;
    case kDtF32: err = launch_matmul<float>(A, B, C, M, K, N, stream); break;
    case kDtF64: err = launch_matmul<double>(A, B, C, M, K, N, stream); break;
    default: return static_cast<int>(cudaErrorInvalidValue);
    }
    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

// 低精度 GEMM 入口（Task 5）：F16/BF16 输入，float 累加，float 输出
int ops_matmul_lowp(int dt, const void* A, const void* B, float* C, std::size_t M, std::size_t K, std::size_t N) noexcept
{
    // 验证 dtype：仅接受 F16 或 BF16
    if (dt != kDtF16 && dt != kDtBF16)
        return static_cast<int>(cudaErrorInvalidValue);

    // M==0 或 N==0 时无输出元素，直接成功
    if (M == 0 || N == 0)
        return 0;

    // K==0 时内积为 0，调用方负责预清零输出；直接返回成功
    if (K == 0)
        return 0;

    // 有效尺寸下指针不得为空
    if (!A || !B || !C)
        return static_cast<int>(cudaErrorInvalidValue);

    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    if (dt == kDtF16) {
        err = launch_matmul_lowp<__half>(A, B, C, M, K, N, stream);
    }
    else {
        err = launch_matmul_lowp<__nv_bfloat16>(A, B, C, M, K, N, stream);
    }

    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail
