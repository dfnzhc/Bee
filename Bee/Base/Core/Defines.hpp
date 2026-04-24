/**
 * @File Defines.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include <new>

#if defined(__has_cpp_attribute)
    #define BEE_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
    #define BEE_HAS_CPP_ATTRIBUTE(x) 0
#endif

#if defined(__has_include)
    #define BEE_HAS_INCLUDE(x) __has_include(x)
#else
    #define BEE_HAS_INCLUDE(x) 0
#endif

#if defined(__has_builtin)
    #define BEE_HAS_BUILTIN(x) __has_builtin(x)
#else
    #define BEE_HAS_BUILTIN(x) 0
#endif

#if BEE_HAS_CPP_ATTRIBUTE(no_unique_address) >= 201803L
    #define BEE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#elif BEE_HAS_CPP_ATTRIBUTE(msvc::no_unique_address) >= 201803L
    #define BEE_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
    #define BEE_NO_UNIQUE_ADDRESS
#endif

#if defined(__cpp_lib_hardware_interference_size) && !defined(__APPLE__)
    #define BEE_CACHE_LINE_SIZE std::hardware_destructive_interference_size
#else
    #define BEE_CACHE_LINE_SIZE 64
#endif

// --- Debugger break ---

#if defined(_MSC_VER)
    #define BEE_DEBUG_BREAK __debugbreak()
#elif BEE_HAS_BUILTIN(__builtin_debugtrap)
    #define BEE_DEBUG_BREAK __builtin_debugtrap()
#elif defined(__linux__) || defined(__APPLE__)
    #include <signal.h>
    #define BEE_DEBUG_BREAK raise(SIGTRAP)
#else
    #define BEE_DEBUG_BREAK ((void)0)
#endif

// --- Compiler optimization hints ---

#if defined(_MSC_VER)
    #define BEE_ASSUME(expr) __assume(expr)
#elif BEE_HAS_BUILTIN(__builtin_assume)
    #define BEE_ASSUME(expr) __builtin_assume(expr)
#elif BEE_HAS_CPP_ATTRIBUTE(assume)
    #define BEE_ASSUME(expr) [[assume(expr)]]
#else
    #define BEE_ASSUME(expr) ((void)0)
#endif

#if BEE_HAS_BUILTIN(__builtin_expect)
    #define BEE_LIKELY(expr)   (__builtin_expect(!!(expr), 1))
    #define BEE_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#else
    #define BEE_LIKELY(expr)   (!!(expr))
    #define BEE_UNLIKELY(expr) (!!(expr))
#endif

// --- Force inline ---

#if defined(_MSC_VER)
    #define BEE_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define BEE_FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define BEE_FORCE_INLINE inline
#endif

// --- Unroll ---

#if defined(__CUDACC__) || defined(__NVCC__) || defined(__clang__)
    #define BEE_UNROLL _Pragma("unroll")
#else
    #define BEE_UNROLL
#endif