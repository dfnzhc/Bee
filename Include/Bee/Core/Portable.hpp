/**
 * @File Portable.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/21
 * @Brief 根据平台、编译器定义特定功能的宏
 */

#pragma once

// -------------------------
// 平台
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#include <intrin.h>
#define BEE_ON_WINDOWS
#elif defined(__unix__) || defined(__unix) || defined(__linux__)
#define BEE_ON_LINUX
#elif defined(__APPLE__) || defined(__MACH__)
#define BEE_ON_MAC
#else
#error "不支持的平台"
#endif

#if BEE_ON_LINUX || BEE_ON_MAC
#warning "目前仅在 Windows 上开发测试"
#endif

// -------------------------
// 编译器检测

// 检测是否在 CUDA 编译环境中 (Host 或 Device)
#if defined(__CUDACC__) || defined(__NVCC__)
#define BEE_COMPILER_NVCC 1
#endif

#if defined(_MSC_VER) && defined(__clang__)
#define BEE_COMPILER_CLANG_CL 1 // MSVC Clang (clang-cl)
#elif defined(_MSC_VER) && !defined(__clang__)
#define BEE_COMPILER_MSVC 1 // MSVC (cl)
#elif defined(__clang__)
#define BEE_COMPILER_CLANG 1 // Standard Clang
#elif defined(__GNUC__) && !defined(__clang__)
#define BEE_COMPILER_GCC 1 // GCC
#else
#ifndef BEE_COMPILER_NVCC
#error "不支持的编译器"
#endif
#endif

// -------------------------
// Cpp version
#if defined(BEE_COMPILER_MSVC) || defined(BEE_COMPILER_CLANG_CL)
#define BEE_CPLUSPLUS _MSVC_LANG
#else
#define BEE_CPLUSPLUS __cplusplus
#endif

#if !defined(__CUDA_ARCH__)
static_assert(BEE_CPLUSPLUS >= 202002L, "Bee 使用现代 C++ 特性, 需要至少 C++20");
#endif

// -------------------------
// 关于 Cuda、Optix 的宏定义
#if defined(__CUDA_ARCH__)
#define BEE_GPU_CODE
#define BEE_GPU __device__
#define BEE_CPU __host__
#define BEE_INLINE __forceinline__
#define BEE_CONST __device__ const
#else
#define BEE_HOST_CODE
#define BEE_GPU /* ignore */
#define BEE_CPU /* ignore */
#define BEE_CONST const
#define BEE_INLINE inline
#endif

#define BEE_CPU_GPU BEE_CPU BEE_GPU

#define BEE_FUNC BEE_INLINE BEE_CPU_GPU
#define BEE_FUNC_DECL BEE_CPU_GPU

// -------------------------
// noinline

#if defined(BEE_GPU_CODE)
#define BEE_NOINLINE __noinline__
#elif defined(BEE_COMPILER_MSVC) || defined(BEE_COMPILER_CLANG_CL)
#define BEE_NOINLINE __declspec(noinline)
#elif defined(BEE_COMPILER_GCC) || defined(BEE_COMPILER_CLANG)
#define BEE_NOINLINE __attribute__((__noinline__))
#else
#define BEE_NOINLINE
#endif

// -------------------------
// force inline
#if defined(BEE_GPU_CODE)
#define BEE_FORCE_INLINE __forceinline__
#elif defined(BEE_COMPILER_MSVC) || defined(BEE_COMPILER_CLANG_CL)
#define BEE_FORCE_INLINE __forceinline
#elif defined(BEE_COMPILER_GCC) || defined(BEE_COMPILER_CLANG)
#define BEE_FORCE_INLINE inline __attribute__((__always_inline__))
#else
#define BEE_FORCE_INLINE inline
#endif

// -------------------------
// always inline
#if defined(BEE_GPU_CODE)
#define BEE_ALWAYS_INLINE __forceinline__
#elif defined(BEE_COMPILER_MSVC) || defined(BEE_COMPILER_CLANG_CL)
#define BEE_ALWAYS_INLINE __forceinline
#elif defined(BEE_COMPILER_GCC) || defined(BEE_COMPILER_CLANG)
#define BEE_ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
#define BEE_ALWAYS_INLINE inline
#endif

// -------------------------
// likely
#if defined(BEE_GPU_CODE) || defined(BEE_COMPILER_GCC) || defined(BEE_COMPILER_CLANG)
#define BEE_LIKELY(x) __builtin_expect(!!(x), 1)
#define BEE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define BEE_LIKELY(x) (x)
#define BEE_UNLIKELY(x) (x)
#endif

// -------------------------
// assume
#if defined(BEE_GPU_CODE)
#define BEE_ASSUME(x) if (!(x)) __builtin_unreachable()
#elif defined(__cpp_attributes) && __cpp_attributes >= 202207L
// C++23 标准属性
#define BEE_ASSUME(x) [[assume(x)]]
#elif defined(BEE_COMPILER_MSVC) || defined(BEE_COMPILER_CLANG_CL)
#define BEE_ASSUME(x) __assume(x)
#elif defined(BEE_COMPILER_CLANG)
#define BEE_ASSUME(x) __builtin_assume(x)
#elif defined(BEE_COMPILER_GCC)
#define BEE_ASSUME(x) __attribute__((assume(x)))
#else
#define BEE_ASSUME(x) ((void)0)
#endif

// -------------------------
// alignment
#define BEE_ALIGNAS(x) alignas(x)

// -------------------------
// debugbreak

#if defined(BEE_GPU_CODE)
#define BEE_DEBUG_BREAK() asm("trap;")
#elif defined(BEE_COMPILER_MSVC) || defined(BEE_COMPILER_CLANG_CL)
#define BEE_DEBUG_BREAK() __debugbreak()
#elif defined(BEE_COMPILER_CLANG) || defined(BEE_COMPILER_GCC)
#if defined(__i386__) || defined(__x86_64__)
#define BEE_DEBUG_BREAK() __asm__ volatile("int $0x03")
#elif defined(__aarch64__)
#define BEE_DEBUG_BREAK() __asm__ volatile(".inst 0xd4200000")
#else
#define BEE_DEBUG_BREAK() __builtin_trap()
#endif
#else
#include <csignal>
#define BEE_DEBUG_BREAK() std::raise(SIGTRAP)
#endif

// -------------------------
// restrict
#if defined(BEE_GPU_CODE)
#define BEE_RESTRICT __restrict__
#elif defined(BEE_COMPILER_MSVC) || defined(BEE_COMPILER_CLANG_CL)
#define BEE_RESTRICT __restrict
#elif defined(BEE_COMPILER_GCC) || defined(BEE_COMPILER_CLANG)
#define BEE_RESTRICT __restrict__
#else
#define BEE_RESTRICT
#endif

// -------------------------
// Pragma Warning

#define _BEE_PRAGMA_STR(x) #x
#define _BEE_PRAGMA(x) _Pragma(#x)

// MSVC Warning
#if defined(BEE_COMPILER_MSVC) || defined(BEE_COMPILER_CLANG_CL)
#define BEE_DISABLE_MSVC_WARNING(id) __pragma(warning(disable : id))
#define BEE_PUSH_WARNING __pragma(warning(push))
#define BEE_POP_WARNING  __pragma(warning(pop))
#else
#define BEE_DISABLE_MSVC_WARNING(id)
#endif

// GCC/Clang Warning
#if defined(BEE_COMPILER_GCC) || defined(BEE_COMPILER_CLANG)
#define BEE_DISABLE_GCC_WARNING(flag) _Pragma("GCC diagnostic ignored \"" flag "\"")
#ifndef BEE_PUSH_WARNING
#define BEE_PUSH_WARNING _Pragma("GCC diagnostic push")
#define BEE_POP_WARNING  _Pragma("GCC diagnostic pop")
#endif
#else
#define BEE_DISABLE_GCC_WARNING(flag)
#endif

// NVCC Warning
#if defined(BEE_COMPILER_NVCC)
// nv_diag_suppress 仅在 .cu 文件中有效
#define BEE_DISABLE_NVCC_WARNING(id) _BEE_PRAGMA(nv_diag_suppress id)
// NVCC 通常也支持 GCC/MSVC 的 push/pop 语法，这里做兜底
#ifndef BEE_PUSH_WARNING
#define BEE_PUSH_WARNING _Pragma("nv_diag_push")
#define BEE_POP_WARNING  _Pragma("nv_diag_pop")
#endif
#else
#define BEE_DISABLE_NVCC_WARNING(id)
#endif

#ifndef BEE_PUSH_WARNING
#define BEE_PUSH_WARNING
#define BEE_POP_WARNING
#endif
