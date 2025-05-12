/**
 * @File MathDefines.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/5/12
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cstdint>
#include <cstddef>

// -------------------------
// Host and device macros

#if defined(__CUDA_ARCH__) || defined(__CUDACC__)
#ifndef BEE_GPU_CODE
#   define BEE_GPU_CODE 1
#endif
#else
#ifndef BEE_HOST_CODE
#   define BEE_HOST_CODE 1
#endif
#endif

#if BEE_HOST_CODE
#include <limits>
#include <type_traits>

// C++20 <bit>
#if __cplusplus >= 202002L && __has_include(<bit>)
#include <bit>
#define BEE_USE_STL_BIT 1
#else
#define BEE_USE_STL_BIT 0
#endif

// Compiler detection for intrinsics
#if defined(__clang__) || defined(__GNUC__)
#define BEE_HAS_GCC_CLANG_INTRINSICS 1
#else
#define BEE_HAS_GCC_CLANG_INTRINSICS 0
#endif

#if defined(_MSC_VER)
#define BEE_HAS_MSVC_INTRINSICS 1
#include <intrin.h> // For MSVC intrinsics (_BitScanForward, _BitScanReverse, __popcnt etc.)
#else
#define BEE_HAS_MSVC_INTRINSICS 0
#endif

#endif

namespace bee {
// -------------------------
// Aliases

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8   = uint8_t;
using u16  = uint16_t;
using u32  = uint32_t;
using u64  = uint64_t;
using uint = unsigned int;

using f32 = float;
using f64 = double;

using Size = std::size_t;

static constexpr i8 kI8Min   = INT8_MIN;
static constexpr i16 kI16Min = INT16_MIN;
static constexpr i32 kI32Min = INT32_MIN;
static constexpr i64 kI64Min = INT64_MIN;

static constexpr i8 kI8Max   = INT8_MAX;
static constexpr i16 kI16Max = INT16_MAX;
static constexpr i32 kI32Max = INT32_MAX;
static constexpr i64 kI64Max = INT64_MAX;

static constexpr u8 kU8Max   = UINT8_MAX;
static constexpr u16 kU16Max = UINT16_MAX;
static constexpr u32 kU32Max = UINT32_MAX;
static constexpr u64 kU64Max = UINT64_MAX;
} // namespace bee