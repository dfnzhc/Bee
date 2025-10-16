/**
 * @File Defines.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/12
 * @Brief This file is part of Bee.
 */

#pragma once

#include "./Portable.hpp"

#include <cstdint>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace Bee
{
    template <typename T>
    using numeric_limits = std::numeric_limits<T>;

    // -------------------------
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
} // namespace Bee

// -----------------------------
// helper macros

#define BEE_UNUSED(x) (void)x

// clang-format off
#define BEE_ENUM_CLASS_OPERATORS(enumType)                                                                                               \
inline constexpr enumType operator& (enumType a, enumType b) { return static_cast<enumType>(static_cast<int>(a)& static_cast<int>(b)); } \
inline constexpr enumType operator| (enumType a, enumType b) { return static_cast<enumType>(static_cast<int>(a)| static_cast<int>(b)); } \
inline constexpr enumType& operator|= (enumType& a, enumType b) { a = a | b; return a; };                                                \
inline constexpr enumType& operator&= (enumType& a, enumType b) { a = a & b; return a; };                                                \
inline constexpr enumType  operator~ (enumType a) { return static_cast<enumType>(~static_cast<int>(a)); }                                \
inline constexpr bool IsSet(enumType val, enumType flag) { return (val & flag) != static_cast<enumType>(0); }                            \
inline constexpr void FlipEnumBit(enumType& val, enumType flag) { val = IsSet(val, flag) ? (val & (~flag)) : (val | flag); }
// clang-format on


#define BEE_DISABLE_COPY(ClassName)                 \
    ClassName(const ClassName&) = delete;           \
    ClassName& operator=(const ClassName&) = delete

#define BEE_DISABLE_COPY_AND_MOVE(ClassName)                \
    ClassName(const ClassName&) = delete;                   \
    ClassName& operator=(const ClassName&) = delete;        \
    ClassName(ClassName&&) = delete;                        \
    ClassName& operator=(ClassName&&) = delete
