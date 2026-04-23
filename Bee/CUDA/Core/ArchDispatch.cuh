/**
 * @File Core/ArchDispatch.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 架构能力宏，用于 __CUDA_ARCH__ 条件编译分派。
 *
 * plan-cuda §2 能力清单：sm_120 (Blackwell)。
 *  - cp.async / cp.async.bulk (TMA)  : 可用
 *  - UMMA (tcgen05.mma)              : 可用（Blackwell 主力）
 *  - WGMMA                            : 不可用（Hopper 专属）
 *  - wmma / mma.sync                  : 可用
 */

#pragma once

// 主架构门：sm_120 及以上。
#ifdef __CUDA_ARCH__
    #define BEE_CUDA_ARCH __CUDA_ARCH__
#else
    #define BEE_CUDA_ARCH 0
#endif

#define BEE_CUDA_ARCH_IS_SM120_PLUS (BEE_CUDA_ARCH >= 1200)
#define BEE_CUDA_ARCH_IS_SM90       (BEE_CUDA_ARCH >= 900 && BEE_CUDA_ARCH < 1000)
#define BEE_CUDA_ARCH_IS_SM80_PLUS  (BEE_CUDA_ARCH >= 800)

// 能力位（宏即结果；未来可按需细分 sm_120a 等）。
#define BEE_CUDA_HAS_CP_ASYNC      (BEE_CUDA_ARCH >= 800)
#define BEE_CUDA_HAS_CP_ASYNC_BULK (BEE_CUDA_ARCH >= 900)
#define BEE_CUDA_HAS_TMA           (BEE_CUDA_ARCH >= 900)
#define BEE_CUDA_HAS_WGMMA         (BEE_CUDA_ARCH >= 900 && BEE_CUDA_ARCH < 1000)
#define BEE_CUDA_HAS_UMMA          (BEE_CUDA_ARCH >= 1200)
#define BEE_CUDA_HAS_WMMA          (BEE_CUDA_ARCH >= 700)
