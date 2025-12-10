/**
 * @File Bits.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include <bit>
#include "Bee/Core/Concepts.hpp"

namespace bee
{
template <ArithType To, ArithType From>
constexpr To BitCast(const From& src) noexcept
{
    return std::bit_cast<To>(src);
}

template <UnsignedType T>
constexpr bool HasSingleBit(const T number) noexcept
{
    return std::has_single_bit(number);
}

template <UnsignedType T>
constexpr T BitCeil(T x) noexcept
{
    return std::bit_ceil(x);
}

template <UnsignedType T>
constexpr T BitFloor(T x) noexcept
{
    return std::bit_floor(x);
}

template <UnsignedType T>
constexpr T BitWidth(T x) noexcept
{
    return std::bit_width(x);
}

template <UnsignedType T>
constexpr T RotateLeft(T x, int s) noexcept
{
    return std::rotl(x, s);
}

template <UnsignedType T>
constexpr T RotateRight(T x, int s) noexcept
{
    return std::rotr(x, s);
}

template <UnsignedType T>
constexpr T CountLeadingZero(T x) noexcept
{
    return std::countl_zero(x);
}

template <UnsignedType T>
constexpr T CountLeadingOne(T x) noexcept
{
    return std::countl_one(x);
}

template <UnsignedType T>
constexpr T CountTrailingZero(T x) noexcept
{
    return std::countr_zero(x);
}

template <UnsignedType T>
constexpr T CountTrailingOne(T x) noexcept
{
    return std::countr_one(x);
}

template <UnsignedType T>
constexpr int OneBitCount(T x) noexcept
{
    return std::popcount(x);
}

template <UnsignedType T>
constexpr int ZeroBitCount(T x) noexcept
{
    static_assert(std::popcount(0xFULL) == 4);
    return sizeof(T) * 8 - std::popcount(x);
}

template <UnsignedType T>
constexpr bool IsPowerOfTwo(T value) noexcept
{
    return HasSingleBit(value);
}

template <UnsignedType T>
constexpr bool IsPowerOf2(T v) noexcept
{
    return v && !(v & (v - 1));
}

template <UnsignedType T>
constexpr T PreviousPowerOfTwo(T value) noexcept
{
    return BitFloor(value);
}

template <UnsignedType T>
constexpr T NextPowerOfTwo(T value) noexcept
{
    return BitCeil(value);
}

template <UnsignedType T>
constexpr T ClosestPowerOfTwo(T value) noexcept
{
    if (value == 0)
        return 1;

    auto nx = NextPowerOfTwo(value);
    auto px = PreviousPowerOfTwo(value);
    return (nx - value) >= (value - px) ? px : nx;
}

template <UnsignedType T>
constexpr T ReverseBits(T value) noexcept
{
    T v = value;
    if constexpr (sizeof(T) == 1)
    {
        // 8-bit
        v = ((v & 0xF0) >> 4) | ((v & 0x0F) << 4);
        v = ((v & 0xCC) >> 2) | ((v & 0x33) << 2);
        v = ((v & 0xAA) >> 1) | ((v & 0x55) << 1);
    }
    else if constexpr (sizeof(T) == 2)
    {
        // 16-bit
        v = ((v & 0xFF00) >> 8) | ((v & 0x00FF) << 8);
        v = ((v & 0xF0F0) >> 4) | ((v & 0x0F0F) << 4);
        v = ((v & 0xCCCC) >> 2) | ((v & 0x3333) << 2);
        v = ((v & 0xAAAA) >> 1) | ((v & 0x5555) << 1);
    }
    else if constexpr (sizeof(T) == 4)
    {
        // 32-bit
        v = ((v & 0xFFFF0000) >> 16) | ((v & 0x0000FFFF) << 16);
        v = ((v & 0xFF00FF00) >> 8) | ((v & 0x00FF00FF) << 8);
        v = ((v & 0xF0F0F0F0) >> 4) | ((v & 0x0F0F0F0F) << 4);
        v = ((v & 0xCCCCCCCC) >> 2) | ((v & 0x33333333) << 2);
        v = ((v & 0xAAAAAAAA) >> 1) | ((v & 0x55555555) << 1);
    }
    else
    {
        // 64-bit
        v = ((v & 0xFFFFFFFF00000000) >> 32) | ((v & 0x00000000FFFFFFFF) << 32);
        v = ((v & 0xFFFF0000FFFF0000) >> 16) | ((v & 0x0000FFFF0000FFFF) << 16);
        v = ((v & 0xFF00FF00FF00FF00) >> 8) | ((v & 0x00FF00FF00FF00FF) << 8);
        v = ((v & 0xF0F0F0F0F0F0F0F0) >> 4) | ((v & 0x0F0F0F0F0F0F0F0F) << 4);
        v = ((v & 0xCCCCCCCCCCCCCCCC) >> 2) | ((v & 0x3333333333333333) << 2);
        v = ((v & 0xAAAAAAAAAAAAAAAA) >> 1) | ((v & 0x5555555555555555) << 1);
    }

    return v;
}

template <UnsignedType T>
constexpr T BitSwap(T value) noexcept
{
    T v = value;
    if constexpr (sizeof(T) == 1)
    {
        return v;
    }
    else if constexpr (sizeof(T) == 2)
    {
        return (v >> 8) | (v << 8);
    }
    else if constexpr (sizeof(T) == 4)
    {
        return ((v << 24) | ((v << 8) & 0x00FF0000) | ((v >> 8) & 0x0000FF00) | (v >> 24));
    }
    else
    {
        v = (v & 0x00000000FFFFFFFF) << 32 | (v & 0xFFFFFFFF00000000) >> 32;
        v = (v & 0x0000FFFF0000FFFF) << 16 | (v & 0xFFFF0000FFFF0000) >> 16;
        v = (v & 0x00FF00FF00FF00FF) << 8 | (v & 0xFF00FF00FF00FF00) >> 8;
        return v;
    }
}

template <UnsignedType T>
constexpr T SetBit(T value, int pos) noexcept
{
    return value | (static_cast<T>(1) << pos);
}

template <UnsignedType T>
constexpr T ClearBit(T value, int pos) noexcept
{
    return value & ~(static_cast<T>(1) << pos);
}

template <UnsignedType T>
constexpr T ToggleBit(T value, int pos) noexcept
{
    return value ^ (static_cast<T>(1) << pos);
}

template <UnsignedType T>
constexpr bool CheckBit(T value, int pos) noexcept
{
    return (value >> pos) & 1;
}

constexpr u32 RoundUp(u32 x, u32 y)
{
    if (x == 0)
        return 0u;
    if (y == 0)
        return x;
    return ((x + y - 1) / y) * y;
}

constexpr u64 AlignUp(u64 value, u64 alignment)
{
    // Assumes alignment is a power of two
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace bee
