/**
 * @File Ops/StridedCopy.cu
 * @Brief 通用 strided copy kernel：将任意步长布局的源 tensor 物化为连续目标缓冲。
 */

#include "CUDA/Ops/OpsBridge.hpp"

#include <cuda_runtime.h>
#include <cstdint>

namespace
{

// 每个线程处理一个输出元素：先把线性输出下标还原为多维坐标，再按
// strides 和 offset_elements 定位源 storage 中的元素。
// args.shape/strides 以末尾维度最快（C-order）解码线性索引。
// 最多支持 BEE_STRIDED_COPY_MAX_NDIM 维。

constexpr int BEE_STRIDED_COPY_MAX_NDIM = 8;

struct StridedCopyParams
{
    int64_t shape[BEE_STRIDED_COPY_MAX_NDIM];
    int64_t strides[BEE_STRIDED_COPY_MAX_NDIM];
    int64_t offset; // storage base offset (in elements)
    int     ndim;
};

template <typename T>
__global__ void strided_copy_kernel(const T* __restrict__ src, T* __restrict__ dst, StridedCopyParams args, std::size_t numel)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= numel)
        return;

    // 从线性索引 i 按末尾维度最快（C-order）解码多维索引，并按 strides 计算 src 偏移
    int64_t linear  = static_cast<int64_t>(i);
    int64_t src_off = args.offset;

    for (int d = args.ndim - 1; d >= 0; --d) {
        int64_t idx  = linear % args.shape[d];
        src_off     += idx * args.strides[d];
        linear      /= args.shape[d];
    }

    dst[i] = src[src_off];
}

template <typename T>
int launch_strided_copy(const void* src, void* dst, StridedCopyParams args, std::size_t numel, cudaStream_t stream)
{
    if (numel == 0)
        return 0;
    constexpr unsigned BLOCK = 256;
    const unsigned     grid  = static_cast<unsigned>((numel + BLOCK - 1) / BLOCK);
    strided_copy_kernel<T><<<grid, BLOCK, 0, stream>>>(static_cast<const T*>(src), static_cast<T*>(dst), args, numel);
    return static_cast<int>(cudaGetLastError());
}

} // namespace

namespace bee::cuda::detail
{

int ops_strided_copy(
    int            dt,
    const void*    src,
    void*          dst,
    const int64_t* shape,
    const int64_t* strides,
    int            ndim,
    int64_t        offset_elements,
    std::size_t    numel
) noexcept
{
    if (numel == 0)
        return 0;
    if (ndim <= 0 || ndim > BEE_STRIDED_COPY_MAX_NDIM)
        return static_cast<int>(cudaErrorInvalidValue);

    // 打包 shape/strides 到 kernel 参数结构（避免额外设备内存分配）
    StridedCopyParams args{};
    for (int d = 0; d < ndim; ++d) {
        args.shape[d]   = shape[d];
        args.strides[d] = strides[d];
    }
    args.offset = offset_elements;
    args.ndim   = ndim;

    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    // 按元素大小分派（dtype 编码：0=Bool/U8, 1=U8, 2=I32, 3=I64, 4=F32, 5=F64）
    switch (dt) {
    case 0: // Bool
    case 1: // U8
        err = launch_strided_copy<std::uint8_t>(src, dst, args, numel, stream);
        break;
    case 2: // I32
        err = launch_strided_copy<std::int32_t>(src, dst, args, numel, stream);
        break;
    case 3: // I64
        err = launch_strided_copy<std::int64_t>(src, dst, args, numel, stream);
        break;
    case 4: // F32
        err = launch_strided_copy<float>(src, dst, args, numel, stream);
        break;
    case 5: // F64
        err = launch_strided_copy<double>(src, dst, args, numel, stream);
        break;
    default: return static_cast<int>(cudaErrorInvalidValue);
    }

    if (err != 0)
        return err;
    // 保持同步可观察语义（与 transpose_2d 等其他 kernel 一致）
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail
