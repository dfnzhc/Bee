/**
 * @File Numeric.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/23
 * @Brief This file is part of Bee.
 */

#pragma once

#include <limits>
#include <numeric>

namespace bee
{

template <typename T>
constexpr T SaturatingAdd(T a, T b) noexcept
{
    const auto maxValue = std::numeric_limits<T>::max();
    if (maxValue - a < b) {
        return maxValue;
    }
    return static_cast<T>(a + b);
}

} // namespace bee
