/**
 * @File Core/ArchDispatch.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 架构能力宏，用于 __CUDA_ARCH__ 条件编译分派。
 *
 * 当前同时覆盖两类目标：
 * - sm_89  (Ada):    支持 WMMA / cp.async，不支持 TMA / UMMA。
 * - sm_120 (Blackwell): 支持 TMA / UMMA / WMMA。
 *
 * 所有设备侧实现都应只依赖本文件暴露的能力宏，而不是在各 TU 内重复硬编码架构号。
 */

#pragma once

// Host 侧不会定义 __CUDA_ARCH__，此时统一视为 0，便于在同一头文件里复用条件编译。
#ifdef __CUDA_ARCH__
    #define BEE_CUDA_ARCH __CUDA_ARCH__
#else
    #define BEE_CUDA_ARCH 0
#endif

#define BEE_CUDA_ARCH_IS_SM89       (BEE_CUDA_ARCH >= 890 && BEE_CUDA_ARCH < 900)
#define BEE_CUDA_ARCH_IS_SM89_PLUS  (BEE_CUDA_ARCH >= 890)
#define BEE_CUDA_ARCH_IS_SM90_PLUS  (BEE_CUDA_ARCH >= 900)
#define BEE_CUDA_ARCH_IS_SM120_PLUS (BEE_CUDA_ARCH >= 1200)
#define BEE_CUDA_ARCH_IS_SM80_PLUS  (BEE_CUDA_ARCH >= 800)

// 能力位（宏即结果；未来可按需细分 sm_120a 等）。
#define BEE_CUDA_HAS_CP_ASYNC      (BEE_CUDA_ARCH >= 800)
#define BEE_CUDA_HAS_CP_ASYNC_BULK (BEE_CUDA_ARCH >= 900)
#define BEE_CUDA_HAS_TMA           (BEE_CUDA_ARCH >= 900)
#define BEE_CUDA_HAS_WGMMA         (BEE_CUDA_ARCH >= 900 && BEE_CUDA_ARCH < 1000)
#define BEE_CUDA_HAS_UMMA          (BEE_CUDA_ARCH >= 1200)
#define BEE_CUDA_HAS_WMMA          (BEE_CUDA_ARCH >= 700)

// 当前 Bee::CUDA Native matmul 的完整能力要求：
// 1. 具备 TMA / TensorMap 搬运能力；
// 2. 具备 WMMA 计算能力；
// 3. 当前仅在 Blackwell 能力层（以 UMMA 可用作为标识）完成实现与验证。
#define BEE_CUDA_HAS_NATIVE_TMA_WMMA (BEE_CUDA_HAS_TMA && BEE_CUDA_HAS_WMMA && BEE_CUDA_HAS_UMMA)
