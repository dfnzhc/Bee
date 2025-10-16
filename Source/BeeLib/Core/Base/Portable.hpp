/**
 * @File Portable.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/13
 * @Brief This file is part of Bee.
 */

#pragma once

// -------------------------
// Platform
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#  include <Windows.h>
#  include <intrin.h>
#  define BEE_ON_WINDOWS
#elif defined(__unix__) || defined(__unix) || defined(__linux__)
#  define BEE_ON_LINUX
#elif defined(__APPLE__) || defined(__MACH__)
#  define BEE_ON_MAC
#else
#  error "Unsupport Platform."
#endif

#if BEE_ON_LINUX || BEE_ON_MAC
#warning "Bee is currently developed on Windows platform."
#endif

// -------------------------
// Compiler
#if defined(_MSC_VER) && !defined(__clang__)
#  define BEE_COMPILER_MSVC 1
#elif defined(__clang__)
#  define BEE_COMPILER_CLANG 1
#elif defined(__GNUC__) && !defined(__clang__)
#  define BEE_COMPILER_GCC 1
#else
#  error "Unknown Compiler."
#endif

// -------------------------
// Cpp version
#if BEE_COMPILER_MSVC
#  define BEE_CPLUSPLUS _MSVC_LANG
#else
#  define BEE_CPLUSPLUS __cplusplus
#endif

static_assert(BEE_CPLUSPLUS >= 202'302L, "Bee using modern C++ features.");

// -------------------------
// noinline
#if BEE_COMPILER_MSVC
#  define BEE_NOINLINE __declspec(noinline)
#elif BEE_COMPILER_CLANG
#  define BEE_NOINLINE __attribute__((__noinline__))
#else
#  define BEE_NOINLINE
#endif

// -------------------------
// always inline
#if BEE_COMPILER_MSVC
#  define BEE_ALWAYS_INLINE __forceinline
#elif BEE_COMPILER_CLANG
#  define BEE_ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
#  define BEE_ALWAYS_INLINE inline
#endif

// -------------------------
// force inline
#if BEE_COMPILER_MSVC
#  define BEE_FORCE_INLINE __forceinline
#elif BEE_COMPILER_GCC || BEE_COMPILER_CLANG
#  define BEE_FORCE_INLINE __attribute__((always_inline)) inline
#else
#  define BEE_FORCE_INLINE inline
#endif

// -------------------------
// Generalize warning push/pop.
#if BEE_COMPILER_CLANG || BEE_COMPILER_GCC
// Clang & GCC
#  define BEE_PUSH_WARNING                               _Pragma("GCC diagnostic push")
#  define BEE_POP_WARNING                                _Pragma("GCC diagnostic pop")
#  define BEE_GNU_DISABLE_WARNING_INTERNAL2(warningName) #warningName
#  define BEE_GNU_DISABLE_WARNING(warningName)           _Pragma(BEE_GNU_DISABLE_WARNING_INTERNAL2(GCC diagnostic ignored warningName))
#  ifdef __clang__
#    define BEE_CLANG_DISABLE_WARNING(warningName) BEE_GNU_DISABLE_WARNING(warningName)
#    define BEE_GCC_DISABLE_WARNING(warningName)
#  else
#    define BEE_CLANG_DISABLE_WARNING(warningName)
#    define BEE_GCC_DISABLE_WARNING(warningName) BEE_GNU_DISABLE_WARNING(warningName)
#  endif
#  define BEE_MSVC_DISABLE_WARNING(warningNumber)
#elif BEE_COMPILER_MSVC
#  define BEE_PUSH_WARNING __pragma(warning(push))
#  define BEE_POP_WARNING  __pragma(warning(pop))
// Disable the GCC warnings.
#  define BEE_GNU_DISABLE_WARNING(warningName)
#  define BEE_GCC_DISABLE_WARNING(warningName)
#  define BEE_CLANG_DISABLE_WARNING(warningName)
#  define BEE_MSVC_DISABLE_WARNING(warningNumber) __pragma(warning(disable : warningNumber))
#else
#  define BEE_PUSH_WARNING
#  define BEE_POP_WARNING
#  define BEE_GNU_DISABLE_WARNING(warningName)
#  define BEE_GCC_DISABLE_WARNING(warningName)
#  define BEE_CLANG_DISABLE_WARNING(warningName)
#  define BEE_MSVC_DISABLE_WARNING(warningNumber)
#endif

// -------------------------
// has builtin
#ifdef __has_builtin
#  define BEE_HAS_BUILTIN(x) __has_builtin(x)
#else
#  define BEE_HAS_BUILTIN(x) 0
#endif

// -------------------------
// likely & unlikely
#if BEE_COMPILER_GCC || BEE_COMPILER_CLANG
#  define BEE_LIKELY(x)   __builtin_expect(!!(x), 1)
#  define BEE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#  define BEE_LIKELY(x)   (x)
#  define BEE_UNLIKELY(x) (x)
#endif

// -------------------------
// assume
#if BEE_COMPILER_MSVC
#  define BEE_ASSUME(condition) __assume(condition)

#elif BEE_COMPILER_CLANG
#  if BEE_HAS_BUILTIN(__builtin_assume)
#    define BEE_ASSUME(condition) __builtin_assume(condition)
#  else
#    define BEE_ASSUME(condition)        \
        do {                             \
            if (!(condition))            \
                __builtin_unreachable(); \
        } while (0)
#  endif

#elif BEE_COMPILER_GCC
#  define BEE_ASSUME(condition)            \
      do {                                 \
          if (BEE_UNLIKELY(!(condition)))  \
              __builtin_unreachable();     \
      } while (0)
#else
#  define BEE_ASSUME(condition) ((void)0)
#endif

// -------------------------
// alignas
#if BEE_COMPILER_MSVC
#  define BEE_ALIGNAS(N) __declspec(align(N))
#elif BEE_COMPILER_GCC || BEE_COMPILER_CLANG
#  define BEE_ALIGNAS(N) __attribute__((aligned(N)))
#else
#  define BEE_ALIGNAS(N) alignas(N)
#endif

// -------------------------
// debug break
#if BEE_ENABLE_DEBUG_BREAK

#  if defined(BEE_ON_WINDOWS) && defined(BEE_COMPILER_MSVC)
#    define BEE_DEBUG_BREAK() __debugbreak()
#  elif BEE_COMPILER_GCC
#    if BEE_HAS_BUILTIN(__builtin_trap)
#      define BEE_DEBUG_BREAK() __builtin_trap()
#    else
#      define BEE_DEBUG_BREAK() __asm__ volatile("int $0x03")
#    endif

#  elif BEE_COMPILER_CLANG
#    if BEE_HAS_BUILTIN(__builtin_debugtrap)
#      define BEE_DEBUG_BREAK() __builtin_debugtrap()
#    else
#      define BEE_DEBUG_BREAK() __asm__ volatile("int $0x03")
#    endif

#  else
#    include <cassert> // Required for assert
#    define BEE_DEBUG_BREAK() assert(false && "BEE_DEBUG_BREAK: Reached generic breakpoint")
#  endif

#else
#  define BEE_DEBUG_BREAK() ((void)0) // No-op in release or CUDA device code by default
#endif

// -------------------------
// restrict
#if BEE_COMPILER_MSVC
#  define BEE_RESTRICT __restrict
#elif BEE_COMPILER_GCC || BEE_COMPILER_CLANG
#  define BEE_RESTRICT __restrict__
#else
#  define BEE_RESTRICT
#endif

// -------------------------
// attributes
#define BEE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#define BEE_NODISCARD         [[nodiscard]]
#define BEE_NORETURN          [[noreturn]]
#define BEE_DEPRECATED(...)   [[deprecated(__VA_ARGS__)]]
#define BEE_FALLTHROUGH(...)  [[fallthrough]]
