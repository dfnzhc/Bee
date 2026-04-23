/**
 * @File Ops/Transpose.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief B9: vectorized float4/uint4 快速路径 + 原 32×32 tile kernel 回退。
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

// ─── vectorized float4 / uint4 fast-path ────────────────────────────────────
// 仅在 rows/cols 均为 32 倍数、且源/目的按 16B 对齐时启用。
// TILE=32, block=(8,32)，每线程 x 方向处理 4 连续元素（float4/uint4）。

template <typename T, typename V4>
__global__ void transpose_vec4_kernel(const T* __restrict__ src, T* __restrict__ dst, std::size_t rows, std::size_t cols)
{
    constexpr int TILE_X = 32; // 物理列数
    __shared__ T  tile[TILE_X][TILE_X + 1];

    const unsigned tx = threadIdx.x; // 0..7，对应 x/4
    const unsigned ty = threadIdx.y; // 0..31，对应 y

    // Read: src[by*TILE + ty][bx*TILE + tx*4..tx*4+3] → tile[ty][tx*4..tx*4+3]
    const std::size_t src_row = static_cast<std::size_t>(blockIdx.y) * TILE_X + ty;
    const std::size_t src_col = static_cast<std::size_t>(blockIdx.x) * TILE_X + tx * 4;
    const V4*         src4    = reinterpret_cast<const V4*>(src + src_row * cols + src_col);
    V4                v       = *src4;
    // 共享内存按 [TILE_X+1] pad，行跨度非 16B 整数倍，仅能做标量写。
    tile[ty][tx * 4 + 0] = reinterpret_cast<const T*>(&v)[0];
    tile[ty][tx * 4 + 1] = reinterpret_cast<const T*>(&v)[1];
    tile[ty][tx * 4 + 2] = reinterpret_cast<const T*>(&v)[2];
    tile[ty][tx * 4 + 3] = reinterpret_cast<const T*>(&v)[3];

    __syncthreads();

    // Write: dst[bx*TILE + ty][by*TILE + tx*4 + k] = tile[tx*4+k][ty], k=0..3
    V4 o;
    reinterpret_cast<T*>(&o)[0] = tile[tx * 4 + 0][ty];
    reinterpret_cast<T*>(&o)[1] = tile[tx * 4 + 1][ty];
    reinterpret_cast<T*>(&o)[2] = tile[tx * 4 + 2][ty];
    reinterpret_cast<T*>(&o)[3] = tile[tx * 4 + 3][ty];

    const std::size_t dst_row = static_cast<std::size_t>(blockIdx.x) * TILE_X + ty;
    const std::size_t dst_col = static_cast<std::size_t>(blockIdx.y) * TILE_X + tx * 4;
    V4*               dst4    = reinterpret_cast<V4*>(dst + dst_row * rows + dst_col);
    *dst4                     = o;
}

template <typename T, typename V4>
int launch_transpose_vec4(const void* src, void* dst, std::size_t rows, std::size_t cols, cudaStream_t stream)
{
    constexpr int TILE_X = 32;
    dim3          block(8, 32); // 256 threads
    dim3          grid(static_cast<unsigned int>(cols / TILE_X), static_cast<unsigned int>(rows / TILE_X));
    transpose_vec4_kernel<T, V4><<<grid, block, 0, stream>>>(static_cast<const T*>(src), static_cast<T*>(dst), rows, cols);
    return static_cast<int>(cudaGetLastError());
}

inline bool aligned16(const void* p) noexcept
{
    return (reinterpret_cast<std::uintptr_t>(p) & 0xF) == 0;
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

    const bool can_vec4 = (rows % 32 == 0) && (cols % 32 == 0) && aligned16(src) && aligned16(dst);

    // 以元素大小分派（dtype 编码为 0..5，对应 Bool/U8/I32/I64/F32/F64）。
    switch (dt) {
    case 0:
    case 1: err = launch_transpose<std::uint8_t>(src, dst, rows, cols, stream); break;
    case 2: // I32
        if (can_vec4)
            err = launch_transpose_vec4<std::int32_t, int4>(src, dst, rows, cols, stream);
        else
            err = launch_transpose<std::int32_t>(src, dst, rows, cols, stream);
        break;
    case 3: err = launch_transpose<std::int64_t>(src, dst, rows, cols, stream); break;
    case 4: // F32
        if (can_vec4)
            err = launch_transpose_vec4<float, float4>(src, dst, rows, cols, stream);
        else
            err = launch_transpose<float>(src, dst, rows, cols, stream);
        break;
    case 5: err = launch_transpose<double>(src, dst, rows, cols, stream); break;
    default: return static_cast<int>(cudaErrorInvalidValue);
    }
    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail

