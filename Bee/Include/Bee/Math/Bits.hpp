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
// ========== 位级转换 ==========

/// 按位转换（保持比特模式不变）
/// 要求 To/From 尺寸一致且满足 std::bit_cast 的约束
template <ArithType To, ArithType From>
constexpr To BitCast(const From& src) noexcept
{
    return std::bit_cast<To>(src);
}

// ========== 单比特操作 ==========

/// 将指定 bit 位置 1（pos 为从 0 开始的位索引）
template <UnsignedType T>
constexpr T SetBit(T value, int pos) noexcept
{
    return value | (static_cast<T>(1) << pos);
}

/// 将指定 bit 位置 0（pos 为从 0 开始的位索引）
template <UnsignedType T>
constexpr T ClearBit(T value, int pos) noexcept
{
    return value & ~(static_cast<T>(1) << pos);
}

/// 翻转指定 bit 位（pos 为从 0 开始的位索引）
template <UnsignedType T>
constexpr T ToggleBit(T value, int pos) noexcept
{
    return value ^ (static_cast<T>(1) << pos);
}

/// 检查指定 bit 位是否为 1（pos 为从 0 开始的位索引）
template <UnsignedType T>
constexpr bool CheckBit(T value, int pos) noexcept
{
    return (value >> pos) & 1;
}

// ========== 位计数与扫描 ==========

/// 是否只有单个 bit 为 1（等价于 std::has_single_bit；0 返回 false）
template <UnsignedType T>
constexpr bool HasSingleBit(const T number) noexcept
{
    return std::has_single_bit(number);
}

/// 统计 1 的个数（population count）
template <UnsignedType T>
constexpr int OneBitCount(T x) noexcept
{
    return std::popcount(x);
}

/// 统计 0 的个数（位宽 - population count）
template <UnsignedType T>
constexpr int ZeroBitCount(T x) noexcept
{
    return sizeof(T) * 8 - std::popcount(x);
}

/// 统计前导 0 的个数（x 为 0 时返回位宽）
template <UnsignedType T>
constexpr T CountLeadingZero(T x) noexcept
{
    return std::countl_zero(x);
}

/// 统计前导 1 的个数（x 全为 1 时返回位宽）
template <UnsignedType T>
constexpr T CountLeadingOne(T x) noexcept
{
    return std::countl_one(x);
}

/// 统计尾随 0 的个数（x 为 0 时返回位宽）
template <UnsignedType T>
constexpr T CountTrailingZero(T x) noexcept
{
    return std::countr_zero(x);
}

/// 统计尾随 1 的个数（x 全为 1 时返回位宽）
template <UnsignedType T>
constexpr T CountTrailingOne(T x) noexcept
{
    return std::countr_one(x);
}

// ========== 2 的幂与位宽 ==========

/// 返回不小于 x 的最小 2 的幂（x==0 时返回 1）
template <UnsignedType T>
constexpr T BitCeil(T x) noexcept
{
    return std::bit_ceil(x);
}

/// 返回不大于 x 的最大 2 的幂（x==0 时返回 0）
template <UnsignedType T>
constexpr T BitFloor(T x) noexcept
{
    return std::bit_floor(x);
}

/// 返回表示 x 所需的 bit 数（x==0 时返回 0）
template <UnsignedType T>
constexpr T BitWidth(T x) noexcept
{
    return std::bit_width(x);
}

/// 是否为 2 的幂（0 返回 false）
template <UnsignedType T>
constexpr bool IsPowerOfTwo(T value) noexcept
{
    return HasSingleBit(value);
}

/// 返回不大于 value 的最大 2 的幂（value==0 时返回 0）
template <UnsignedType T>
constexpr T PreviousPowerOfTwo(T value) noexcept
{
    return BitFloor(value);
}

/// 返回不小于 value 的最小 2 的幂（value==0 时返回 1）
template <UnsignedType T>
constexpr T NextPowerOfTwo(T value) noexcept
{
    return BitCeil(value);
}

/// 返回距离 value 最近的 2 的幂；距离相同则返回较小者
template <UnsignedType T>
constexpr T ClosestPowerOfTwo(T value) noexcept
{
    if (value == 0)
        return 1;

    auto nx = NextPowerOfTwo(value);
    auto px = PreviousPowerOfTwo(value);
    return (nx - value) >= (value - px) ? px : nx;
}

// ========== 位旋转 ==========

/// 循环左移（rotate left）
template <UnsignedType T>
constexpr T RotateLeft(T x, int s) noexcept
{
    return std::rotl(x, s);
}

/// 循环右移（rotate right）
template <UnsignedType T>
constexpr T RotateRight(T x, int s) noexcept
{
    return std::rotr(x, s);
}

// ========== 反转 ==========

/// 反转 bit 顺序（支持 8/16/32/64 位无符号类型）
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

/// 反转字节序（endianness swap）
template <UnsignedType T>
constexpr T ReverseBytes(T value) noexcept
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

// ========== 对齐与取整 ==========

/// 将 x 向上取整到 y 的倍数（y==0 时返回 x）
constexpr u32 RoundUp(u32 x, u32 y)
{
    if (y == 0)
        return x;
    if (x == 0)
        return 0u;

    u32 remainder = x % y;
    return (remainder == 0) ? x : (x - remainder + y);
}

/// 将 value 向上对齐到 alignment（假设 alignment 为 2 的幂）
constexpr u64 AlignUp(u64 value, u64 alignment)
{
    // Assumes alignment is a power of two
    return (value + alignment - 1) & ~(alignment - 1);
}
} // namespace bee
