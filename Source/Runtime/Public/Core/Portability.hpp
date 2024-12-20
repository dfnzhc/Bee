/**
 * @File Portability.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#  include <Windows.h>
#  include <intrin.h>
#  define BEE_IN_WINDOWS
//#elif defined(__unix__) || defined(__unix) || defined(__linux__)
//#  define BEE_IN_LINUX
//#elif defined(__APPLE__) || defined(__MACH__)
//#  define BEE_IN_MAC
#else
#  error Unsupport Platform
#endif

// noinline
#ifdef _MSC_VER
#  define BEE_NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
#  define BEE_NOINLINE __attribute__((__noinline__))
#else
#  define BEE_NOINLINE
#endif

// always inline
#ifdef _MSC_VER
#  define BEE_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__)
#  define BEE_ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
#  define BEE_ALWAYS_INLINE inline
#endif

#if defined(_MSC_VER)
#  define BEE_MSVC_DECLSPEC(...) __declspec(__VA_ARGS__)
#else
#  define BEE_MSVC_DECLSPEC(...)
#endif

#if defined(_MSC_VER)
#  define BEE_CPLUSPLUS _MSVC_LANG
#else
#  define BEE_CPLUSPLUS __cplusplus
#endif

static_assert(BEE_CPLUSPLUS >= 202'002L, "__cplusplus >= 202002L: C++20 at least");

#define BEE_NODISCARD       [[nodiscard]]
#define BEE_DEPRECATED(...) [[deprecated(__VA_ARGS__)]]

#define BEE_UNUSED(...) void(0)

// Generalize warning push/pop.
#if defined(__GNUC__) || defined(__clang__)
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
#elif defined(_MSC_VER)
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