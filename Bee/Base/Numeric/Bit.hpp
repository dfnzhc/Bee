#pragma once

#include <bit>
#include <cstddef>
#include <concepts>
#include <limits>
#include <type_traits>

namespace bee
{

template <std::unsigned_integral UInt>
[[nodiscard]] constexpr auto IsPowerOfTwo(UInt value) noexcept -> bool
{
    return std::has_single_bit(value);
}

/// Rounds value up to the nearest power of two.
/// Special cases: RoundUpPowerOfTwo(0) == 1, returns 0 on overflow.
template <std::unsigned_integral UInt>
[[nodiscard]] constexpr auto RoundUpPowerOfTwo(UInt value) noexcept -> UInt
{
    if (value <= UInt{1}) {
        return UInt{1};
    }

    constexpr UInt max_power_of_two = std::bit_floor((std::numeric_limits<UInt>::max)());
    if (value > max_power_of_two) {
        return UInt{0};
    }

    return std::bit_ceil(value);
}

template <std::unsigned_integral UInt>
[[nodiscard]] constexpr auto HighestPowerOfTwoLEQ(UInt value) noexcept -> UInt
{
    return std::bit_floor(value);
}

template <std::integral Int>
[[nodiscard]] constexpr auto ByteSwap(Int value) noexcept -> Int
{
    using UInt = std::make_unsigned_t<Int>;

#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L // C++23
    return static_cast<Int>(std::byteswap(static_cast<UInt>(value)));
#else
    UInt input  = static_cast<UInt>(value);
    UInt output = 0;
    for (std::size_t i = 0; i < sizeof(UInt); ++i) {
        output   = static_cast<UInt>((output << 8) | (input & static_cast<UInt>(0xFFu)));
        input  >>= 8;
    }
    return static_cast<Int>(output);
#endif
}

[[nodiscard]] constexpr bool IsBigEndian() noexcept
{
    return std::endian::native == std::endian::big;
}

[[nodiscard]] constexpr bool IsLittleEndian() noexcept
{
    return std::endian::native == std::endian::little;
}

template <std::integral Int>
[[nodiscard]] constexpr auto ToBigEndian(Int value) noexcept -> Int
{
    if constexpr (std::endian::native == std::endian::big) {
        return value;
    } else {
        return ByteSwap(value);
    }
}

template <std::integral Int>
[[nodiscard]] constexpr auto ToLittleEndian(Int value) noexcept -> Int
{
    if constexpr (std::endian::native == std::endian::little) {
        return value;
    } else {
        return ByteSwap(value);
    }
}

template <std::integral Int>
[[nodiscard]] constexpr auto FromBigEndian(Int value) noexcept -> Int
{
    // Byte-swap is its own inverse: FromBigEndian == ToBigEndian.
    return ToBigEndian(value);
}

template <std::integral Int>
[[nodiscard]] constexpr auto FromLittleEndian(Int value) noexcept -> Int
{
    // Byte-swap is its own inverse: FromLittleEndian == ToLittleEndian.
    return ToLittleEndian(value);
}

} // namespace bee
