/**
 * @File Basic.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/23
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Math/MathTraits.hpp"

namespace Bee
{
    template <ArithmeticType T, ArithmeticType U, ArithmeticType... Ts>
        requires ConvertibleTo<U, T> && AllConvertibleTo<T, Ts...>
    constexpr T Min(T a, U b, Ts... vals) noexcept
    {
        const auto m = As<T>(a < b ? a : b);
        if constexpr (sizeof...(vals) > 0)
        {
            return Min(m, As<T>(vals)...);
        }
        return m;
    }

    template <ArithmeticType T, ArithmeticType U, ArithmeticType... Ts>
        requires ConvertibleTo<U, T> && AllConvertibleTo<T, Ts...>
    constexpr T Max(T a, U b, Ts... vals) noexcept
    {
        const auto m = As<T>(a > b ? a : b);
        if constexpr (sizeof...(vals) > 0)
        {
            return Max(m, As<T>(vals)...);
        }
        return m;
    }
} // namespace Bee
