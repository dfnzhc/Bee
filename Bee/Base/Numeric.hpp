/**
 * @File Numeric.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/23
 * @Brief This file is part of Bee.
 */

#pragma once

#include <concepts>
#include <limits>
#include <numeric>

namespace bee
{

template <std::unsigned_integral T>
[[nodiscard]] constexpr T SaturatingAdd(T a, T b) noexcept
{
    const auto maxValue = std::numeric_limits<T>::max();
    if (maxValue - a < b) {
        return maxValue;
    }
    return static_cast<T>(a + b);
}

} // namespace bee
