/**
 * @File Defines.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#pragma once

// -------------------------

#if defined(_MSC_VER)
#  define BEE_API_EXPORT __declspec(dllexport)
#  define BEE_API_IMPORT __declspec(dllimport)
#elif defined(__clang__) || defined(__GNUC__)
#  define BEE_API_EXPORT __attribute__((visibility("default")))
#  define BEE_API_IMPORT __attribute__((visibility("default")))
#else
#  define BEE_API_EXPORT
#  define BEE_API_IMPORT
#endif

#ifdef BEE_DLL
#  define BEE_API BEE_API_EXPORT
#else  // BEE_DLL
#  define BEE_API BEE_API_IMPORT
#endif // BEE_DLL

#include <cstdint>
#include <cstddef>

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


#include <optional>

namespace bee {
template<typename T>
using Opt = std::optional<T>;

template<typename A, typename B>
using OptPair = Opt<std::pair<A, B>>;
} // namespace bee