/**
 * @File Ops/Matmul.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 通用 tiled shared-memory GEMM，实现 Bee::CUDA 的基础 matmul 路径。
 *
 * 本文件承担所有设备都可用的 baseline 行为：
 * - 支持 F32/F64/I32/I64；
 * - 正确处理非 TILE 整倍数的边界；
 * - 更高阶的 CUTLASS / Native(TMA) 路径放在独立 TU 中，由上层显式选择或自动回退。
 */

#include "CUDA/Ops/OpsBridge.hpp"
#include "CUDA/Core/Launch.cuh"

#include <cuda_runtime.h>
#include <cstdint>

namespace
{

constexpr int kDtI32 = 2;
constexpr int kDtI64 = 3;
constexpr int kDtF32 = 4;
constexpr int kDtF64 = 5;

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

} // namespace bee::cuda::detail
