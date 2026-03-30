/**
 * @File Transpose.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cuda_runtime.h>

#include <cstddef>
#include <type_traits>

namespace bee::cuda
{

// =========================================================
// 矩阵转置算子（Row-Major）
// ---------------------------------------------------------
// 输入:  in  形状 [rows, cols]
// 输出: out 形状 [cols, rows]
// 语义: out[c, r] = in[r, c]
//
// 这里提供两条实现路径：
// 1) Tiled: shared memory + padding，解决转置读写中的 bank conflict。
// 2) TiledDiagonal: 在 Tiled 基础上加入对角线 block 重排，缓解 partition camping。
//
// 在绝大多数中大型矩阵上，TiledDiagonal 是最稳定的高性能路径。
// =========================================================

namespace internal
{
    constexpr int kTileDim   = 32;
    constexpr int kBlockRows = 8;

} // namespace internal

template <typename T>
__global__ void transpose_naive_kernel(const T* in, T* out, int rows, int cols)
{
    const int c = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int r = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);
    if (r < rows && c < cols) {
        out[c * rows + r] = in[r * cols + c];
    }
}

template <typename T>
__global__ void transpose_tiled_kernel(const T* in, T* out, int rows, int cols)
{
    using namespace internal;
    __shared__ T tile[kTileDim][kTileDim + 1];

    const int x = static_cast<int>(blockIdx.x) * kTileDim + static_cast<int>(threadIdx.x);
    const int y = static_cast<int>(blockIdx.y) * kTileDim + static_cast<int>(threadIdx.y);

#pragma unroll
    for (int j = 0; j < kTileDim; j += kBlockRows) {
        if (x < cols && (y + j) < rows) {
            tile[threadIdx.y + j][threadIdx.x] = in[(y + j) * cols + x];
        }
    }
    __syncthreads();

    const int ox = static_cast<int>(blockIdx.y) * kTileDim + static_cast<int>(threadIdx.x);
    const int oy = static_cast<int>(blockIdx.x) * kTileDim + static_cast<int>(threadIdx.y);

#pragma unroll
    for (int j = 0; j < kTileDim; j += kBlockRows) {
        if (ox < rows && (oy + j) < cols) {
            out[(oy + j) * rows + ox] = tile[threadIdx.x][threadIdx.y + j];
        }
    }
}

template <typename T>
__global__ void transpose_tiled_diagonal_kernel(const T* in, T* out, int rows, int cols)
{
    using namespace internal;
    __shared__ T tile[kTileDim][kTileDim + 1];

    // 对角线 block 重排：避免 block 线性排布时集中打到同一内存分区。
    int bx = 0;
    int by = 0;
    if (rows == cols) {
        bx = static_cast<int>((blockIdx.x + blockIdx.y) % gridDim.x);
        by = static_cast<int>(blockIdx.x);
    } else {
        const int bid = static_cast<int>(blockIdx.x + gridDim.x * blockIdx.y);
        by            = bid % static_cast<int>(gridDim.y);
        bx            = (bid / static_cast<int>(gridDim.y) + by) % static_cast<int>(gridDim.x);
    }

    const int x = bx * kTileDim + static_cast<int>(threadIdx.x);
    const int y = by * kTileDim + static_cast<int>(threadIdx.y);

#pragma unroll
    for (int j = 0; j < kTileDim; j += kBlockRows) {
        if (x < cols && (y + j) < rows) {
            tile[threadIdx.y + j][threadIdx.x] = in[(y + j) * cols + x];
        }
    }
    __syncthreads();

    const int ox = by * kTileDim + static_cast<int>(threadIdx.x);
    const int oy = bx * kTileDim + static_cast<int>(threadIdx.y);

#pragma unroll
    for (int j = 0; j < kTileDim; j += kBlockRows) {
        if (ox < rows && (oy + j) < cols) {
            out[(oy + j) * rows + ox] = tile[threadIdx.x][threadIdx.y + j];
        }
    }
}

template <typename T>
cudaError_t launch_transpose(const T* in, T* out, int rows, int cols, cudaStream_t stream = nullptr)
{
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, int>, "Launch currently supports float/double/int.");

    if (rows <= 0 || cols <= 0) {
        return cudaSuccess;
    }

    using namespace internal;

    constexpr dim3 block(kTileDim, kBlockRows, 1);
    const dim3 grid((cols + kTileDim - 1) / kTileDim, (rows + kTileDim - 1) / kTileDim, 1);

    const std::size_t elements = static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols);
    if (elements <= 4096) {

        transpose_tiled_kernel<T><<<grid, block, 0, stream>>>(in, out, rows, cols);
        return cudaGetLastError();
    }

    transpose_tiled_diagonal_kernel<T><<<grid, block, 0, stream>>>(in, out, rows, cols);
    return cudaGetLastError();
}

} // namespace bee::cuda
