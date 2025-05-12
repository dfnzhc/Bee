/**
 * @File Bits.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/5/12
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Math/Math.hpp"
#include "Core/Math/MathDefines.hpp"

namespace bee {
namespace detail {
/// @brief Returns the number of bits in type T.
/// @tparam T An unsigned integer type.
/// @return The number of bits in T.
template<typename T>
BEE_FUNC BEE_CONSTEXPR int BitSize()
{
    static_assert(std::is_unsigned_v<T>, "Type T must be an unsigned integer.");
    return std::numeric_limits<T>::digits;
}
} // namespace detail

/**
 * @brief Counts the number of leading zeros in an unsigned integer.
 * @tparam T An unsigned integer type.
 * @param value The value to count leading zeros for.
 * @return The number of leading zeros. Returns BitSize<T>() if value is 0.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR int CountLeadingZeros(T value)
{
    if (value == 0)
        return detail::BitSize<T>();

#if BEE_USE_STL_BIT

    return std::countl_zero(value);
#elif BEE_HAS_GCC_CLANG_INTRINSICS
    if constexpr (sizeof(T) == sizeof(unsigned int)) return __builtin_clz(value);
    if constexpr (sizeof(T) == sizeof(unsigned long)) return __builtin_clzl(value);
    if constexpr (sizeof(T) == sizeof(unsigned long long)) return __builtin_clzll(value);
    // Fallback for types not directly supported by __builtin_clz* (e.g. uint8_t, uint16_t)
    // Promote to unsigned int, count, then adjust.
    if constexpr (sizeof(T) < sizeof(unsigned int)) {
        return __builtin_clz(static_cast<unsigned int>(value)) - (detail::BitSize<unsigned int>() - detail::BitSize<T>());
    }
#elif BEE_HAS_MSVC_INTRINSICS
    unsigned long index;
    if constexpr (sizeof(T) <= sizeof(unsigned long)) { // Handles uint8_t, uint16_t, uint32_t
        if (_BitScanReverse(&index, static_cast<unsigned long>(value))) return detail::BitSize<T>() - 1 - index;
    } else if constexpr (sizeof(T) == sizeof(unsigned __int64)) {
        if (_BitScanReverse64(&index, value)) return detail::BitSize<T>() - 1 - index;
    }
#endif
    // Fallback implementation
    int count  = 0;
    T msb_mask = static_cast<T>(1) << (detail::BitSize<T>() - 1);
    while (msb_mask != 0 && (value & msb_mask) == 0) {
        count++;
        msb_mask >>= 1;
    }
    return count;
}


/**
 * @brief Counts the number of trailing zeros in an unsigned integer.
 * @tparam T An unsigned integer type.
 * @param value The value to count trailing zeros for.
 * @return The number of trailing zeros. Returns BitSize<T>() if value is 0.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR int CountTrailingZeros(T value)
{
    if (value == 0)
        return detail::BitSize<T>();

#if BEE_USE_STL_BIT
    return std::countr_zero(value);
#elif BEE_HAS_GCC_CLANG_INTRINSICS
    if constexpr (sizeof(T) == sizeof(unsigned int)) return __builtin_ctz(value);
    if constexpr (sizeof(T) == sizeof(unsigned long)) return __builtin_ctzl(value);
    if constexpr (sizeof(T) == sizeof(unsigned long long)) return __builtin_ctzll(value);
     // For smaller types, __builtin_ctz on the value itself should work as it looks at lower bits.
    if constexpr (sizeof(T) < sizeof(unsigned int)) return __builtin_ctz(static_cast<unsigned int>(value));
#elif BEE_HAS_MSVC_INTRINSICS
    unsigned long index;
    if constexpr (sizeof(T) <= sizeof(unsigned long)) {
        if (_BitScanForward(&index, static_cast<unsigned long>(value))) return index;
    } else if constexpr (sizeof(T) == sizeof(unsigned __int64)) {
        if (_BitScanForward64(&index, value)) return index;
    }
#endif
    // Fallback implementation
    int count = 0;
    for (int i = 0; i < detail::BitSize<T>(); ++i) {
        if (!((value >> i) & 1)) {
            count++;
        }
        else {
            break;
        }
    }
    return count;
}

/**
 * @brief Counts the number of set bits (1s) in an unsigned integer.
 * @tparam T An unsigned integer type.
 * @param value The value to count set bits for.
 * @return The number of set bits.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR int Popcount(T value)
{
#if BEE_USE_STL_BIT
    return std::popcount(value);
#elif BEE_HAS_GCC_CLANG_INTRINSICS
    if constexpr (sizeof(T) == sizeof(unsigned int)) return __builtin_popcount(value);
    if constexpr (sizeof(T) == sizeof(unsigned long)) return __builtin_popcountl(value);
    if constexpr (sizeof(T) == sizeof(unsigned long long)) return __builtin_popcountll(value);
    // For smaller types, __builtin_popcount should work.
    if constexpr (sizeof(T) < sizeof(unsigned int)) return __builtin_popcount(static_cast<unsigned int>(value));
#elif BEE_HAS_MSVC_INTRINSICS
    if constexpr (sizeof(T) == sizeof(unsigned short)) return __popcnt16(static_cast<unsigned short>(value));
    if constexpr (sizeof(T) == sizeof(unsigned int)) return __popcnt(static_cast<unsigned int>(value));
    if constexpr (sizeof(T) == sizeof(unsigned __int64)) return __popcnt64(value);
#endif
    // Fallback (Brian Kernighan's algorithm)
    int count = 0;
    while (value != 0) {
        value &= (value - 1);
        count++;
    }
    return count;
}

/**
 * @brief Alias for Popcount. Counts the number of set bits (1s).
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return The number of set bits.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR int CountSetBits(T value)
{
    return Popcount(value);
}

/**
 * @brief Counts the number of cleared bits (0s) in an unsigned integer.
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return The number of cleared bits.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR int CountClearedBits(T value)
{
    return detail::BitSize<T>() - Popcount(value);
}

/**
 * @brief Calculates the parity of an unsigned integer.
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return 1 if the number of set bits is odd, 0 if even.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR int Parity(T value)
{
#if BEE_HAS_GCC_CLANG_INTRINSICS && !defined(__clang__) // __builtin_parity seems more common in GCC
    // Clang might not always have __builtin_parity for all types, or it's less documented.
    // For GCC:
    if constexpr (sizeof(T) == sizeof(unsigned int)) return __builtin_parity(value);
    if constexpr (sizeof(T) == sizeof(unsigned long)) return __builtin_parityl(value);
    if constexpr (sizeof(T) == sizeof(unsigned long long)) return __builtin_parityll(value);
#endif
    return Popcount(value) % 2;
}

/**
 * @brief Checks if an unsigned integer has exactly one bit set (i.e., is a power of two).
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return True if value has a single bit set, false otherwise. False for 0.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR bool HasSingleBit(T value)
{
#if BEE_USE_STL_BIT
    return std::has_single_bit(value);
#else
    return value != 0 && (value & (value - 1)) == 0;
#endif
}

/**
 * @brief Checks if an unsigned integer is a power of two. (Alias for HasSingleBit).
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return True if value is a power of two, false otherwise. False for 0.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR bool IsPowerOfTwo(T value)
{
    return HasSingleBit(value);
}

/**
 * @brief Returns the largest power of two not greater than value.
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return The largest power of two <= value. Returns 0 if value is 0.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T BitFloor(T value)
{
    if (value == 0)
        return 0;
#if BEE_USE_STL_BIT
    return std::bit_floor(value);
#else
    // Fallback: set all bits below MSB to 1, then (value XOR (value >> 1))
    // Or more simply: find MSB and return 1 << position_of_MSB
    // Or, using CLZ:
    return static_cast<T>(1) << (detail::BitSize<T>() - 1 - CountLeadingZeros(value));
#endif
}

/**
 * @brief Alias for BitFloor.
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return The largest power of two <= value.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T PreviousPowerOfTwo(T value)
{
    return BitFloor(value);
}

/**
 * @brief Calculates floor(log2(value)).
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return floor(log2(value)). Returns -1 if value is 0.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR int FloorLog2(T value)
{
    if (value == 0)
        return -1;
    return detail::BitSize<T>() - 1 - CountLeadingZeros(value);
}

/**
/// @brief Calculates the number of bits required to represent value.
/// @tparam T An unsigned integer type.
/// @param value The value.
/// @return 0 if value is 0, otherwise FloorLog2(value) + 1.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR int BitWidth(T value)
{
#if BEE_USE_STL_BIT
    return std::bit_width(value);
#else
    if (value == 0) return 0;
    return FloorLog2(value) + 1;
#endif
}

/**
 * @brief Returns the smallest power of two not less than value.
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return The smallest power of two >= value. Returns 1 if value is 0.
 *         Returns 0 if the next power of two would overflow type T (matches std::bit_ceil).
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T BitCeil(T value)
{
    if (value <= 1)
        return 1;
#if BEE_USE_STL_BIT
    return std::bit_ceil(value);
#else
    // TODO: ...
#endif
}

/**
 * @brief Alias for BitCeil.
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return The smallest power of two >= value.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T NextPowerOfTwo(T value)
{
    return BitCeil(value);
}

/**
 * @brief Returns the closest power of two.
 * @tparam T An unsigned integer type.
 * @param value The value.
 * @return The closest power of two.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T ClosestPowerOfTwo(T value)
{
    auto nx = NextPowerOfTwo(value);
    auto px = PreviousPowerOfTwo(value);
    return (nx - value) > (value - px) ? px : nx;
}

/**
 * @brief Reverses the order of bits in an unsigned integer.
 * @tparam T An unsigned integer type.
 * @param value The value whose bits are to be reversed.
 * @return The value with bits reversed.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T ReverseBits(T value)
{
    T v = value;
    if constexpr (sizeof(T) == 1) {
        // 8-bit
        v = ((v & 0xF0) >> 4) | ((v & 0x0F) << 4);
        v = ((v & 0xCC) >> 2) | ((v & 0x33) << 2);
        v = ((v & 0xAA) >> 1) | ((v & 0x55) << 1);
    }
    else if constexpr (sizeof(T) == 2) {
        // 16-bit
        v = ((v & 0xFF00) >> 8) | ((v & 0x00FF) << 8);
        v = ((v & 0xF0F0) >> 4) | ((v & 0x0F0F) << 4);
        v = ((v & 0xCCCC) >> 2) | ((v & 0x3333) << 2);
        v = ((v & 0xAAAA) >> 1) | ((v & 0x5555) << 1);
    }
    else if constexpr (sizeof(T) == 4) {
        // 32-bit
        v = ((v & 0xFFFF0000) >> 16) | ((v & 0x0000FFFF) << 16);
        v = ((v & 0xFF00FF00) >> 8) | ((v & 0x00FF00FF) << 8);
        v = ((v & 0xF0F0F0F0) >> 4) | ((v & 0x0F0F0F0F) << 4);
        v = ((v & 0xCCCCCCCC) >> 2) | ((v & 0x33333333) << 2);
        v = ((v & 0xAAAAAAAA) >> 1) | ((v & 0x55555555) << 1);
    }
    else if constexpr (sizeof(T) == 8) {
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

/**
 * @brief Swap the bits of the byte in an unsigned integer.
 * @tparam T An unsigned integer type.
 * @param value The value whose bits are to be swapped.
 * @return The value with bits swapped.
 */
template<std::unsigned_integral T>
BEE_FUNC BEE_CONSTEXPR T BitSwap(T value)
{
    T v = value;
    if constexpr (sizeof(T) == 1) {
        return v;
    }
    else if constexpr (sizeof(T) == 2) {
        return (v >> 8) | (v << 8);
    }
    else if constexpr (sizeof(T) == 4) {
        return ((v << 24) | ((v << 8) & 0x00FF0000) | ((v >> 8) & 0x0000FF00) | (v >> 24));
    }
    else {
        v = (v & 0x00000000FFFFFFFF) << 32 | (v & 0xFFFFFFFF00000000) >> 32;
        v = (v & 0x0000FFFF0000FFFF) << 16 | (v & 0xFFFF0000FFFF0000) >> 16;
        v = (v & 0x00FF00FF00FF00FF) << 8 | (v & 0xFF00FF00FF00FF00) >> 8;
        return v;
    }
}

/**
 * @brief Rotates bits to the left.
 * @tparam T An unsigned integer type.
 * @param value The value to rotate.
 * @param count The number of positions to rotate left. Negative count rotates right.
 * @return The value after left rotation.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T RotateLeft(T value, int count)
{
    constexpr int bits = detail::BitSize<T>();
    if (bits == 0)
        return value; // Should not happen for standard unsigned types

    count %= bits; // Normalize count to be within [-(bits-1), bits-1]
    if (count < 0) {
        count += bits; // Convert negative count to positive equivalent for right rotation becoming left
    }
    // Now count is in [0, bits-1]

#if BEE_USE_STL_BIT
    return std::rotl(value, count);
#else
    if (count == 0) return value;
    return (value << count) | (value >> (bits - count));
#endif
}

/**
 * @brief Rotates bits to the right.
 * @tparam T An unsigned integer type.
 * @param value The value to rotate.
 * @param count The number of positions to rotate right. Negative count rotates left.
 * @return The value after right rotation.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T RotateRight(T value, int count)
{
    constexpr int bits = detail::BitSize<T>();
    if (bits == 0)
        return value;

    count %= bits;
    if (count < 0) {
        count += bits;
    }
    // Now count is in [0, bits-1]

#if BEE_USE_STL_BIT
    return std::rotr(value, count);
#else
    if (count == 0) return value;
    return (value >> count) | (value << (bits - count));
#endif
}

/**
 * @brief Sets the bit at the specified position to 1.
 * @tparam T An unsigned integer type.
 * @param value The original value.
 * @param pos The 0-indexed position of the bit to set.
 * @return The value with the bit at pos set to 1.
 * @note Behavior is undefined if pos is out of range [0, BitSize<T>() - 1].
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T SetBit(T value, int pos)
{
    // if (pos < 0 || pos >= detail::BitSize<T>()) throw std::out_of_range("Bit position out of range.");
    return value | (static_cast<T>(1) << pos);
}

/**
 * @brief Clears the bit at the specified position to 0.
 * @tparam T An unsigned integer type.
 * @param value The original value.
 * @param pos The 0-indexed position of the bit to clear.
 * @return The value with the bit at pos cleared to 0.
 * @note Behavior is undefined if pos is out of range [0, BitSize<T>() - 1].
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T ClearBit(T value, int pos)
{
    // if (pos < 0 || pos >= detail::BitSize<T>()) throw std::out_of_range("Bit position out of range.");
    return value & ~(static_cast<T>(1) << pos);
}

/**
 * @brief Toggles the bit at the specified position.
 * @tparam T An unsigned integer type.
 * @param value The original value.
 * @param pos The 0-indexed position of the bit to toggle.
 * @return The value with the bit at pos toggled.
 * @note Behavior is undefined if pos is out of range [0, BitSize<T>() - 1].
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T ToggleBit(T value, int pos)
{
    // if (pos < 0 || pos >= detail::BitSize<T>()) throw std::out_of_range("Bit position out of range.");
    return value ^ (static_cast<T>(1) << pos);
}

/**
 * @brief Checks the state of the bit at the specified position.
 * @tparam T An unsigned integer type.
 * @param value The value to check.
 * @param pos The 0-indexed position of the bit to check.
 * @return True if the bit at pos is 1, false if it is 0.
 * @note Behavior is undefined if pos is out of range [0, BitSize<T>() - 1].
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR bool CheckBit(T value, int pos)
{
    // if (pos < 0 || pos >= detail::BitSize<T>()) throw std::out_of_range("Bit position out of range.");
    return (value >> pos) & 1;
}

/**
 * @brief Rounds x up to the nearest multiple of y.
 * @tparam T An unsigned integer type.
 * @param x The value to round up.
 * @param y The divisor to round up to; must be greater than 0.
 * @return The smallest multiple of y that is >= x.
 */
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T RoundUp(T x, T y)
{
    if (x == 0)
        return y;
    if (y == 0) {
        return y;
    }

    return ((x + y - 1) / y) * y;
}

/// @brief Aligns value up to the nearest alignment boundary.
/// @tparam T An unsigned integer type.
/// @param value The value to align.
/// @param alignment The alignment boundary; must be a power of two and > 0.
/// @return The smallest multiple of alignment that is >= value.
template<UnsignedType T>
BEE_FUNC BEE_CONSTEXPR T AlignUp(T value, T alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}
} // namespace bee