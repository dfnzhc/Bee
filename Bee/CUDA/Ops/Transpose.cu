/**
 * @File Ops/Transpose.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief M8 baseline: tiled shared-memory 2D transpose (no bank conflict).
 */

#include "CUDA/Ops/OpsBridge.hpp"

#include <cuda_runtime.h>
#include <cstdint>

namespace
{

constexpr int TILE       = 32;
constexpr int BLOCK_ROWS = 8;

template <typename T>
__global__ void transpose_kernel(const T* __restrict__ src, T* __restrict__ dst, std::size_t rows, std::size_t cols)
{
    __shared__ T tile[TILE][TILE + 1];

    std::size_t x = static_cast<std::size_t>(blockIdx.x) * TILE + threadIdx.x;
    std::size_t y = static_cast<std::size_t>(blockIdx.y) * TILE + threadIdx.y;

    for (int j = 0; j < TILE; j += BLOCK_ROWS) {
        std::size_t yy = y + j;
        if (x < cols && yy < rows)
            tile[threadIdx.y + j][threadIdx.x] = src[yy * cols + x];
    }
    __syncthreads();

    std::size_t x2 = static_cast<std::size_t>(blockIdx.y) * TILE + threadIdx.x;
    std::size_t y2 = static_cast<std::size_t>(blockIdx.x) * TILE + threadIdx.y;

    for (int j = 0; j < TILE; j += BLOCK_ROWS) {
        std::size_t yy = y2 + j;
        if (x2 < rows && yy < cols)
            dst[yy * rows + x2] = tile[threadIdx.x][threadIdx.y + j];
    }
}

template <typename T>
int launch_transpose(const void* src, void* dst, std::size_t rows, std::size_t cols, cudaStream_t stream)
{
    dim3 block(TILE, BLOCK_ROWS);
    dim3 grid(static_cast<unsigned int>((cols + TILE - 1) / TILE), static_cast<unsigned int>((rows + TILE - 1) / TILE));
    transpose_kernel<T><<<grid, block, 0, stream>>>(static_cast<const T*>(src), static_cast<T*>(dst), rows, cols);
    return static_cast<int>(cudaGetLastError());
}

} // namespace

namespace bee::cuda::detail
{

int ops_transpose_2d(int dt, const void* src, void* dst, std::size_t rows, std::size_t cols) noexcept
{
    if (rows == 0 || cols == 0)
        return 0;
    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;
    // 以元素大小分派（dtype 编码为 0..5，对应 Bool/U8/I32/I64/F32/F64）。
    switch (dt) {
    case 0:
    case 1: err = launch_transpose<std::uint8_t>(src, dst, rows, cols, stream); break;
    case 2: err = launch_transpose<std::int32_t>(src, dst, rows, cols, stream); break;
    case 3: err = launch_transpose<std::int64_t>(src, dst, rows, cols, stream); break;
    case 4: err = launch_transpose<float>(src, dst, rows, cols, stream); break;
    case 5: err = launch_transpose<double>(src, dst, rows, cols, stream); break;
    default: return static_cast<int>(cudaErrorInvalidValue);
    }
    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail
