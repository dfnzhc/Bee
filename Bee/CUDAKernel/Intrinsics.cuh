/**
 * @File Intrinsics.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cuda_fp16.h>
#include <cuda_runtime.h>
#include <mma.h>
#include <cstdint>

#include "Warp.cuh"

namespace bee::cuda
{

// =========================================================
// 常用 PTX 指令
// =========================================================

// ----------------------- 特殊寄存器读取 -----------------------

// 当前线程在 CTA 内所属 warp id。
__device__ __forceinline__ int warp_id()
{
    int v;
    asm volatile("mov.u32 %0, %%warpid;" : "=r"(v));
    return v;
}

// 当前线程所在 SM id。
__device__ __forceinline__ int sm_id()
{
    int v;
    asm volatile("mov.u32 %0, %%smid;" : "=r"(v));
    return v;
}

// 设备可见的 SM 数量（硬件寄存器值）。
__device__ __forceinline__ int num_sms()
{
    int v;
    asm volatile("mov.u32 %0, %%nsmid;" : "=r"(v));
    return v;
}

// ----------------------- 时钟与计时 -----------------------

// 32-bit 时钟计数器。
__device__ __forceinline__ uint32_t clock32()
{
    uint32_t v;
    asm volatile("mov.u32 %0, %%clock;" : "=r"(v));
    return v;
}

// 64-bit 时钟计数器。
__device__ __forceinline__ uint64_t clock64()
{
    uint64_t v;
    asm volatile("mov.u64 %0, %%clock64;" : "=l"(v));
    return v;
}

// 全局计时器（架构支持时可用）。
__device__ __forceinline__ uint64_t global_timer()
{
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 700
    uint64_t v;
    asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(v));
    return v;
#else
    return static_cast<uint64_t>(clock64());
#endif
}

// ----------------------- 位操作 -----------------------

// 位反转（bit reverse）。
__device__ __forceinline__ uint32_t brev(uint32_t x)
{
    uint32_t v;
    asm volatile("brev.b32 %0, %1;" : "=r"(v) : "r"(x));
    return v;
}

// 统计 32-bit 中 1 的个数（population count）。
__device__ __forceinline__ int popc(uint32_t x)
{
    int v;
    asm volatile("popc.b32 %0, %1;" : "=r"(v) : "r"(x));
    return v;
}

// 统计前导 0 个数（count leading zeros）。
__device__ __forceinline__ int clz(uint32_t x)
{
    int v;
    asm volatile("clz.b32 %0, %1;" : "=r"(v) : "r"(x));
    return v;
}

// 查找最低位 1 的位置，PTX 返回 1-based，若无则返回 0。
__device__ __forceinline__ int ffs_1based(uint32_t x)
{
    if (x == 0u) {
        return 0;
    }
    // ffs(x) = clz(brev(x)) + 1
    return clz(brev(x)) + 1;
}

// ----------------------- 内存栅栏 -----------------------

// CTA 作用域内存栅栏。
__device__ __forceinline__ void membar_cta()
{
    asm volatile("membar.cta;" ::: "memory");
}

// GPU 作用域内存栅栏（设备内可见）。
__device__ __forceinline__ void membar_gl()
{
    asm volatile("membar.gl;" ::: "memory");
}

// 系统作用域内存栅栏（设备与主机/其他设备可见）。
__device__ __forceinline__ void membar_sys()
{
    asm volatile("membar.sys;" ::: "memory");
}

// =========================================================
// wmma
// =========================================================

__host__ __device__ constexpr bool has_wmma_support()
{
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 700
    return true;
#else
    return false;
#endif
}

__device__ inline void wmma_gemm_m16_n16_k16_f16(const half* a, const half* b, float* c)
{
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 700
    using namespace nvcuda;
    wmma::fragment<wmma::matrix_a, 16, 16, 16, half, wmma::row_major> a_frag;
    wmma::fragment<wmma::matrix_b, 16, 16, 16, half, wmma::col_major> b_frag;
    wmma::fragment<wmma::accumulator, 16, 16, 16, float> c_frag;

    wmma::fill_fragment(c_frag, 0.0f);
    wmma::load_matrix_sync(a_frag, a, 16);
    wmma::load_matrix_sync(b_frag, b, 16);
    wmma::mma_sync(c_frag, a_frag, b_frag, c_frag);
    wmma::store_matrix_sync(c, c_frag, 16, wmma::mem_row_major);
#else
    // Scalar fallback on non-WMMA architectures for API compatibility and validation.
    if (threadIdx.x == 0) {
        for (int i = 0; i < 16; ++i) {
            for (int j = 0; j < 16; ++j) {
                float sum = 0.0f;
                for (int k = 0; k < 16; ++k) {
                    const float av  = __half2float(a[i * 16 + k]);
                    const float bv  = __half2float(b[k + j * 16]);
                    sum            += av * bv;
                }
                c[i * 16 + j] = sum;
            }
        }
    }
#endif
}

// =========================================================
// wgmma
// =========================================================

/**
 * @brief 当前编译目标是否具备 WGMMA 可用条件。
 *
 * 条件：
 * 1) 设备架构至少为 sm90；
 * 2) 编译期检测到 CUTLASS/WGMMA 相关能力宏。
 */
__host__ __device__ constexpr bool has_wgmma_support()
{
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 900 && defined(BEE_HAS_WGMMA) && BEE_HAS_WGMMA
    return true;
#else
    return false;
#endif
}

/**
 * @brief 将共享内存地址或步长编码为 WGMMA descriptor 的 18-bit 片段。
 */
__host__ __device__ __forceinline__ uint64_t matrix_descriptor_encode(uint64_t x)
{
    return ((x & 0x3FFFFu) >> 0x4);
}

/**
 * @brief 构造 WGMMA 共享内存 descriptor。
 *
 * @param ptr           共享内存基地址。
 * @param leading_bytes B 矩阵 leading dimension 的字节步长。
 */
template <typename T>
__device__ __forceinline__ uint64_t make_smem_desc(T* ptr, uint64_t leading_bytes)
{
    const auto addr  = static_cast<uint32_t>(__cvta_generic_to_shared(ptr));
    uint64_t desc    = 0;
    desc            |= matrix_descriptor_encode(addr);
    desc            |= matrix_descriptor_encode(16ull) << 16;
    desc            |= matrix_descriptor_encode(leading_bytes) << 32;
    desc            |= 1ull << 62; // 128B swizzle
    return desc;
}

/**
 * @brief WGMMA 指令序列前的 fence，同步 descriptor 与共享内存可见性。
 */
__device__ __forceinline__ void wgmma_fence()
{
#if __CUDA_ARCH__ >= 900
    asm volatile("wgmma.fence.sync.aligned;\n" ::: "memory");
#endif
}

/**
 * @brief 提交当前 warp-group 的异步 WGMMA 指令组。
 */
static __device__ __forceinline__ void wgmma_commit_group()
{
#if __CUDA_ARCH__ >= 900
    asm volatile("wgmma.commit_group.sync.aligned;\n" ::: "memory");
#endif
}

/**
 * @brief 等待最多 N 组尚未完成的 WGMMA 指令。
 */
template <int N>
static __device__ __forceinline__ void wgmma_wait_group()
{
#if __CUDA_ARCH__ >= 900
    static_assert(N >= 0 && N <= 7, "wgmma.wait_group expects [0,7]");
    asm volatile("wgmma.wait_group.sync.aligned %0;\n" : : "n"(N) : "memory");
#else
    (void)N;
#endif
}

/**
 * @brief m64n8k16/f32 累加的 WGMMA 原子封装，作为其他 N 形状实现参考模板。
 */
template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n8_k16_f32(float d[4], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n8k16.f32.f16.f16 "
                 "{%0,%1,%2,%3}, %4, %5, %6, %7, %8, %9, %10;\n"
                 "}\n"
                 : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

/**
 * @brief m64n16..m64n64（step=8）的 f32 累加 WGMMA 封装。
 *
 * 约定：d 的长度与 N 成正比（N/2 个 float）。
 */
template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n16_k16_f32(float d[8], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n16k16.f32.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7}, %8, %9, %10, %11, %12, %13, %14;\n"
                 "}\n"
                 : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3]), "+f"(d[4]), "+f"(d[5]), "+f"(d[6]), "+f"(d[7])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n24_k16_f32(float d[12], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n24k16.f32.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11}, %12, %13, %14, %15, %16, %17, %18;\n"
                 "}\n"
                 : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3]), "+f"(d[4]), "+f"(d[5]), "+f"(d[6]), "+f"(d[7]), "+f"(d[8]), "+f"(d[9]),
                   "+f"(d[10]), "+f"(d[11])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n32_k16_f32(float d[16], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n32k16.f32.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15}, %16, %17, %18, %19, %20, %21, %22;\n"
                 "}\n"
                 : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3]), "+f"(d[4]), "+f"(d[5]), "+f"(d[6]), "+f"(d[7]), "+f"(d[8]), "+f"(d[9]),
                   "+f"(d[10]), "+f"(d[11]), "+f"(d[12]), "+f"(d[13]), "+f"(d[14]), "+f"(d[15])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n40_k16_f32(float d[20], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n40k16.f32.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17,%18,%19}, %20, %21, %22, %23, %24, %25, %26;\n"
                 "}\n"
                 : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3]), "+f"(d[4]), "+f"(d[5]), "+f"(d[6]), "+f"(d[7]), "+f"(d[8]), "+f"(d[9]),
                   "+f"(d[10]), "+f"(d[11]), "+f"(d[12]), "+f"(d[13]), "+f"(d[14]), "+f"(d[15]), "+f"(d[16]), "+f"(d[17]), "+f"(d[18]), "+f"(d[19])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n48_k16_f32(float d[24], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n48k16.f32.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17,%18,%19,%20,%21,%22,%23}, %24, %25, %26, %27, %28, %29, %30;\n"
                 "}\n"
                 : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3]), "+f"(d[4]), "+f"(d[5]), "+f"(d[6]), "+f"(d[7]), "+f"(d[8]), "+f"(d[9]),
                   "+f"(d[10]), "+f"(d[11]), "+f"(d[12]), "+f"(d[13]), "+f"(d[14]), "+f"(d[15]), "+f"(d[16]), "+f"(d[17]), "+f"(d[18]), "+f"(d[19]),
                   "+f"(d[20]), "+f"(d[21]), "+f"(d[22]), "+f"(d[23])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n56_k16_f32(float d[28], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n56k16.f32.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17,%18,%19,%20,%21,%22,%23,%24,%25,%26,%27}, %28, %29, %30, %31, %32, "
                 "%33, %34;\n"
                 "}\n"
                 : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3]), "+f"(d[4]), "+f"(d[5]), "+f"(d[6]), "+f"(d[7]), "+f"(d[8]), "+f"(d[9]),
                   "+f"(d[10]), "+f"(d[11]), "+f"(d[12]), "+f"(d[13]), "+f"(d[14]), "+f"(d[15]), "+f"(d[16]), "+f"(d[17]), "+f"(d[18]), "+f"(d[19]),
                   "+f"(d[20]), "+f"(d[21]), "+f"(d[22]), "+f"(d[23]), "+f"(d[24]), "+f"(d[25]), "+f"(d[26]), "+f"(d[27])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n64_k16_f32(float d[32], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n64k16.f32.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17,%18,%19,%20,%21,%22,%23,%24,%25,%26,%27,%28,%29,%30,%31}, %32, %33, "
                 "%34, %35, %36, %37, %38;\n"
                 "}\n"
                 : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3]), "+f"(d[4]), "+f"(d[5]), "+f"(d[6]), "+f"(d[7]), "+f"(d[8]), "+f"(d[9]),
                   "+f"(d[10]), "+f"(d[11]), "+f"(d[12]), "+f"(d[13]), "+f"(d[14]), "+f"(d[15]), "+f"(d[16]), "+f"(d[17]), "+f"(d[18]), "+f"(d[19]),
                   "+f"(d[20]), "+f"(d[21]), "+f"(d[22]), "+f"(d[23]), "+f"(d[24]), "+f"(d[25]), "+f"(d[26]), "+f"(d[27]), "+f"(d[28]), "+f"(d[29]),
                   "+f"(d[30]), "+f"(d[31])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

/**
 * @brief m64n8..m64n64（step=8）的 f16 累加 WGMMA 封装。
 *
 * 约定：d 采用 packed half（uint32_t）寄存器承载。
 */
template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n8_k16_f16(uint32_t d[2], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n8k16.f16.f16.f16 "
                 "{%0,%1}, %2, %3, %4, %5, %6, %7, %8;\n"
                 "}\n"
                 : "+r"(d[0]), "+r"(d[1])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n16_k16_f16(uint32_t d[4], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n16k16.f16.f16.f16 "
                 "{%0,%1,%2,%3}, %4, %5, %6, %7, %8, %9, %10;\n"
                 "}\n"
                 : "+r"(d[0]), "+r"(d[1]), "+r"(d[2]), "+r"(d[3])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n24_k16_f16(uint32_t d[6], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n24k16.f16.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5}, %6, %7, %8, %9, %10, %11, %12;\n"
                 "}\n"
                 : "+r"(d[0]), "+r"(d[1]), "+r"(d[2]), "+r"(d[3]), "+r"(d[4]), "+r"(d[5])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n32_k16_f16(uint32_t d[8], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n32k16.f16.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7}, %8, %9, %10, %11, %12, %13, %14;\n"
                 "}\n"
                 : "+r"(d[0]), "+r"(d[1]), "+r"(d[2]), "+r"(d[3]), "+r"(d[4]), "+r"(d[5]), "+r"(d[6]), "+r"(d[7])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n40_k16_f16(uint32_t d[10], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n40k16.f16.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9}, %10, %11, %12, %13, %14, %15, %16;\n"
                 "}\n"
                 : "+r"(d[0]), "+r"(d[1]), "+r"(d[2]), "+r"(d[3]), "+r"(d[4]), "+r"(d[5]), "+r"(d[6]), "+r"(d[7]), "+r"(d[8]), "+r"(d[9])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n48_k16_f16(uint32_t d[12], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n48k16.f16.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11}, %12, %13, %14, %15, %16, %17, %18;\n"
                 "}\n"
                 : "+r"(d[0]), "+r"(d[1]), "+r"(d[2]), "+r"(d[3]), "+r"(d[4]), "+r"(d[5]), "+r"(d[6]), "+r"(d[7]), "+r"(d[8]), "+r"(d[9]),
                   "+r"(d[10]), "+r"(d[11])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n56_k16_f16(uint32_t d[14], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n56k16.f16.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13}, %14, %15, %16, %17, %18, %19, %20;\n"
                 "}\n"
                 : "+r"(d[0]), "+r"(d[1]), "+r"(d[2]), "+r"(d[3]), "+r"(d[4]), "+r"(d[5]), "+r"(d[6]), "+r"(d[7]), "+r"(d[8]), "+r"(d[9]),
                   "+r"(d[10]), "+r"(d[11]), "+r"(d[12]), "+r"(d[13])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_n64_k16_f16(uint32_t d[16], const uint64_t desc_a, const uint64_t desc_b)
{
#if __CUDA_ARCH__ >= 900
    asm volatile("{\n"
                 "wgmma.mma_async.sync.aligned.m64n64k16.f16.f16.f16 "
                 "{%0,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15}, %16, %17, %18, %19, %20, %21, %22;\n"
                 "}\n"
                 : "+r"(d[0]), "+r"(d[1]), "+r"(d[2]), "+r"(d[3]), "+r"(d[4]), "+r"(d[5]), "+r"(d[6]), "+r"(d[7]), "+r"(d[8]), "+r"(d[9]),
                   "+r"(d[10]), "+r"(d[11]), "+r"(d[12]), "+r"(d[13]), "+r"(d[14]), "+r"(d[15])
                 : "l"(desc_a), "l"(desc_b), "n"(int32_t(ScaleD)), "n"(int32_t(ScaleA)), "n"(int32_t(ScaleB)), "n"(int32_t(TransA)),
                   "n"(int32_t(TransB)));
#else
    (void)d;
    (void)desc_a;
    (void)desc_b;
#endif
}

template <int N>
struct F32Regs
{
    static_assert(N % 8 == 0 && N >= 8 && N <= 256, "N must be in [8,256] and divisible by 8");
    static constexpr int kCount = N / 2;
};

template <int N>
struct F16Regs
{
    static_assert(N % 8 == 0 && N >= 8 && N <= 256, "N must be in [8,256] and divisible by 8");
    static constexpr int kCount = N / 4;
};

/**
 * @brief m64n(8..256, step=8) 的 f32 累加统一入口，内部自动按 64 列分块。
 */
template <int N, int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_nk16_f32(float* d, half* sA, half* sB, uint64_t ldb_bytes)
{
#if __CUDA_ARCH__ >= 900
    static_assert(N % 8 == 0 && N >= 8 && N <= 256, "N must be in [8,256] and divisible by 8");
    if constexpr (N <= 64) {
        const uint64_t desc_a = make_smem_desc(sA, 1024ull);
        const uint64_t desc_b = make_smem_desc(sB, ldb_bytes);
        if constexpr (N == 8)
            wgmma_mma_m64_n8_k16_f32<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<float (*)[4]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 16)
            wgmma_mma_m64_n16_k16_f32<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<float (*)[8]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 24)
            wgmma_mma_m64_n24_k16_f32<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<float (*)[12]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 32)
            wgmma_mma_m64_n32_k16_f32<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<float (*)[16]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 40)
            wgmma_mma_m64_n40_k16_f32<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<float (*)[20]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 48)
            wgmma_mma_m64_n48_k16_f32<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<float (*)[24]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 56)
            wgmma_mma_m64_n56_k16_f32<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<float (*)[28]>(d)[0], desc_a, desc_b);
        else
            wgmma_mma_m64_n64_k16_f32<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<float (*)[32]>(d)[0], desc_a, desc_b);
    } else {
        constexpr int kChunk = 64;
        constexpr int kTail  = N - kChunk;
        wgmma_mma_m64_nk16_f32<kChunk, ScaleD, ScaleA, ScaleB, TransA, TransB>(d, sA, sB, ldb_bytes);
        wgmma_mma_m64_nk16_f32<kTail, ScaleD, ScaleA, ScaleB, TransA, TransB>(d + F32Regs<kChunk>::kCount, sA, sB + kChunk, ldb_bytes);
    }
#else
    (void)d;
    (void)sA;
    (void)sB;
    (void)ldb_bytes;
#endif
}

/**
 * @brief m64n(8..256, step=8) 的 f16 累加统一入口，内部自动按 64 列分块。
 */
template <int N, int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_nk16_f16(uint32_t* d, half* sA, half* sB, uint64_t ldb_bytes)
{
#if __CUDA_ARCH__ >= 900
    static_assert(N % 8 == 0 && N >= 8 && N <= 256, "N must be in [8,256] and divisible by 8");
    const uint64_t desc_a = make_smem_desc(sA, 1024ull);
    if constexpr (N <= 64) {
        const uint64_t desc_b = make_smem_desc(sB, ldb_bytes);
        if constexpr (N == 8)
            wgmma_mma_m64_n8_k16_f16<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<uint32_t (*)[2]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 16)
            wgmma_mma_m64_n16_k16_f16<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<uint32_t (*)[4]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 24)
            wgmma_mma_m64_n24_k16_f16<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<uint32_t (*)[6]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 32)
            wgmma_mma_m64_n32_k16_f16<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<uint32_t (*)[8]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 40)
            wgmma_mma_m64_n40_k16_f16<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<uint32_t (*)[10]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 48)
            wgmma_mma_m64_n48_k16_f16<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<uint32_t (*)[12]>(d)[0], desc_a, desc_b);
        else if constexpr (N == 56)
            wgmma_mma_m64_n56_k16_f16<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<uint32_t (*)[14]>(d)[0], desc_a, desc_b);
        else
            wgmma_mma_m64_n64_k16_f16<ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<uint32_t (*)[16]>(d)[0], desc_a, desc_b);
    } else {
        constexpr int kChunk = 64;
        constexpr int kTail  = N - kChunk;
        wgmma_mma_m64_nk16_f16<kChunk, ScaleD, ScaleA, ScaleB, TransA, TransB>(d, sA, sB, ldb_bytes);
        wgmma_mma_m64_nk16_f16<kTail, ScaleD, ScaleA, ScaleB, TransA, TransB>(d + F16Regs<kChunk>::kCount, sA, sB + kChunk, ldb_bytes);
    }
#else
    (void)d;
    (void)sA;
    (void)sB;
    (void)ldb_bytes;
#endif
}

enum class DType
{
    F16,
    F32,
};

/**
 * @brief WGMMA 统一入口：根据 Acc 类型分发到 f32/f16 累加实现。
 *
 * N 支持范围为 [8,256] 且必须是 8 的倍数。
 */
template <int N, DType Acc = DType::F32, int ScaleD = 1, int ScaleA = 1, int ScaleB = 1, int TransA = 0, int TransB = 0>
static __device__ __forceinline__ void wgmma_mma_m64_nk16(void* d, half* sA, half* sB, uint64_t ldb_bytes)
{
    static_assert(N % 8 == 0 && N >= 8 && N <= 256, "N must be in [8,256] and divisible by 8");
    if constexpr (Acc == DType::F32) {
        wgmma_mma_m64_nk16_f32<N, ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<float*>(d), sA, sB, ldb_bytes);
    } else {
        wgmma_mma_m64_nk16_f16<N, ScaleD, ScaleA, ScaleB, TransA, TransB>(reinterpret_cast<uint32_t*>(d), sA, sB, ldb_bytes);
    }
}

} // namespace bee::cuda
