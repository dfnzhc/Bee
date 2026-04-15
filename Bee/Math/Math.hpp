#pragma once

#include <cassert>
#include <cmath>
#include <concepts>
#include <type_traits>

namespace bee
{

template <std::totally_ordered T>
[[nodiscard]] constexpr T Clamp(T value, T low, T high) noexcept
{
    assert(low <= high && "Clamp: requires low <= high");
    return value < low ? low : (value > high ? high : value);
}

template <typename T, typename U>
    requires std::is_arithmetic_v<T> && std::is_arithmetic_v<U>
[[nodiscard]] constexpr auto Lerp(T from, T to, U t) noexcept
{
    if constexpr (std::is_floating_point_v<T> && std::is_floating_point_v<U>) {
        using CommonT = std::common_type_t<T, U>;
        return static_cast<CommonT>(std::lerp(static_cast<CommonT>(from), static_cast<CommonT>(to), static_cast<CommonT>(t)));
    } else {
        return from + (to - from) * t;
    }
}

} // namespace bee
