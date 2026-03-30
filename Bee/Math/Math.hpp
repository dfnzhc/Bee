#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace bee
{

template <typename T>
constexpr T Clamp(T value, T low, T high) noexcept
{
    return value < low ? low : (value > high ? high : value);
}

template <typename T, typename U>
requires std::is_arithmetic_v<T> && std::is_arithmetic_v<U>
constexpr auto Lerp(T from, T to, U t) noexcept
{
    return from + (to - from) * t;
}

constexpr bool IsPowerOfTwo(std::uint64_t value) noexcept
{
    return value != 0 && (value & (value - 1)) == 0;
}

} // namespace bee
