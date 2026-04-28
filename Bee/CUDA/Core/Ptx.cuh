/**
 * @File Core/Ptx.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 低层 PTX 内联包装（cp.async 等）。
 *
 * 当前只封装本组件已使用的 cp.async 等基础 PTX 原语；更高阶矩阵指令由
 * 专门的 matmul 后端按架构能力单独实现。
 * 所有 inline 汇编仅在设备代码（__CUDA_ARCH__ 可用）中展开。
 */

#pragma once

#include <cuda_runtime.h>

#include "CUDA/Core/ArchDispatch.cuh"

namespace bee::cuda::ptx
{

// cp.async: 从 global 异步拷贝到 shared memory（单线程一条指令）。
// size_bytes 必须为 4/8/16；地址 16B 对齐时可走 CG 快路径。
// plan §6.1 软件流水线基础。
template <int SizeBytes>
__device__ __forceinline__ void cp_async_cg(void* smem_dst, const void* gmem_src)
{
    static_assert(SizeBytes == 4 || SizeBytes == 8 || SizeBytes == 16, "cp.async size must be 4, 8, or 16 bytes");
#if BEE_CUDA_HAS_CP_ASYNC
    const unsigned int smem_ptr = __cvta_generic_to_shared(smem_dst);
    asm volatile("cp.async.cg.shared.global [%0], [%1], %2;\n" : : "r"(smem_ptr), "l"(gmem_src), "n"(SizeBytes));
#else
    // 非支持架构：退化为同步拷贝（仅用于占位，运行期一般不会到达）。
    auto*       d = static_cast<char*>(smem_dst);
    const auto* s = static_cast<const char*>(gmem_src);
    for (int i = 0; i < SizeBytes; ++i)
        d[i] = s[i];
#endif
}

// cp.async.commit_group：将此前发射的 cp.async 归入一个组。
__device__ __forceinline__ void cp_async_commit_group()
{
#if BEE_CUDA_HAS_CP_ASYNC
    asm volatile("cp.async.commit_group;\n");
#endif
}

// 等待至多 N 个尚未完成的 cp.async 组。
template <int N>
__device__ __forceinline__ void cp_async_wait_group()
{
#if BEE_CUDA_HAS_CP_ASYNC
    asm volatile("cp.async.wait_group %0;\n" ::"n"(N));
#endif
}

__device__ __forceinline__ void cp_async_wait_all()
{
#if BEE_CUDA_HAS_CP_ASYNC
    asm volatile("cp.async.wait_all;\n");
#endif
}

} // namespace bee::cuda::ptx
