/**
 * @File Common.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Math/Math.hpp"
#include "Core/Math/Constant.hpp"

namespace bee {
#define BEE_DEFINE_COMMON_FUNC(Type, Name, Func)                                                                                                     \
    template<Type T> BEE_FUNC BEE_CONSTEXPR T Name(T x) noexcept                                                                                     \
    {                                                                                                                                                \
        return glm::Func(x);                                                                                                                         \
    }

BEE_DEFINE_COMMON_FUNC(cFloatType, Floor, floor)
BEE_DEFINE_COMMON_FUNC(cFloatType, Ceil, ceil)
BEE_DEFINE_COMMON_FUNC(cFloatType, Trunc, trunc)
BEE_DEFINE_COMMON_FUNC(cFloatType, Round, round)
BEE_DEFINE_COMMON_FUNC(cFloatType, Sqrt, sqrt)
BEE_DEFINE_COMMON_FUNC(cFloatType, Exp, exp)
BEE_DEFINE_COMMON_FUNC(cFloatType, Exp2, exp2)
BEE_DEFINE_COMMON_FUNC(cFloatType, Log, log)
BEE_DEFINE_COMMON_FUNC(cFloatType, Log2, log2)
BEE_DEFINE_COMMON_FUNC(cFloatType, Sin, sin)
BEE_DEFINE_COMMON_FUNC(cFloatType, Cos, cos)
BEE_DEFINE_COMMON_FUNC(cFloatType, Tan, tan)
BEE_DEFINE_COMMON_FUNC(cFloatType, Sinh, sinh)
BEE_DEFINE_COMMON_FUNC(cFloatType, Cosh, cosh)
BEE_DEFINE_COMMON_FUNC(cFloatType, Tanh, tanh)
BEE_DEFINE_COMMON_FUNC(cFloatType, ASin, asin)
BEE_DEFINE_COMMON_FUNC(cFloatType, ACos, acos)
BEE_DEFINE_COMMON_FUNC(cFloatType, ATan, atan)
BEE_DEFINE_COMMON_FUNC(cFloatType, ASinh, asinh)
BEE_DEFINE_COMMON_FUNC(cFloatType, ACosh, acosh)
BEE_DEFINE_COMMON_FUNC(cFloatType, ATanh, atanh)

#undef BEE_DEFINE_COMMON_FUNC

template<cFloatType T> BEE_INLINE BEE_CONSTEXPR T Log10(T x) noexcept
{
    return std::log10(x);
}

template<cFloatType T> BEE_INLINE BEE_CONSTEXPR T ATan2(T y, T x) noexcept
{
    return std::atan2(y, x);
}

template<cSignedType T> BEE_FUNC BEE_CONSTEXPR T Abs(T x) noexcept
{
    return glm::abs(x);
}

template<cSignedType T> BEE_FUNC BEE_CONSTEXPR T Sign(T x) noexcept
{
    return glm::sign(x);
}

template<typename T, typename... Ts> BEE_FUNC BEE_CONSTEXPR auto Min(T a, T b, Ts... vals)
{
    const auto m = a < b ? a : b;
    if constexpr (sizeof...(vals) > 0) {
        return Min(m, T(vals)...);
    }

    return m;
}

template<typename T, typename... Ts> BEE_FUNC BEE_CONSTEXPR auto Max(T a, T b, Ts... vals)
{
    const auto m = a > b ? a : b;
    if constexpr (sizeof...(vals) > 0) {
        return Max(m, T(vals)...);
    }

    return m;
}

template<cArithmeticType T> BEE_FUNC BEE_CONSTEXPR bool Equal(T x, T y) noexcept
{
    return x == y;
}

template<cArithmeticType T> BEE_FUNC BEE_CONSTEXPR bool NotEqual(T x, T y) noexcept
{
    return not Equal(x, y);
}

template<cFloatType T> BEE_FUNC BEE_CONSTEXPR bool Approx(T x, T y, f32 eps = kEpsilonF) noexcept
{
    return Abs(x - y) < eps;
}

template<cFloatType T> BEE_FUNC BEE_CONSTEXPR bool NotApprox(T x, T y, f32 eps = kEpsilonF) noexcept
{
    return not Approx(x, y, eps);
}

BEE_FUNC BEE_CONSTEXPR bool IsNaN(std::floating_point auto v)
{
    return std::isnan(v);
}

BEE_FUNC BEE_CONSTEXPR bool IsNaN(auto)
{
    return false;
}

BEE_FUNC BEE_CONSTEXPR bool IsInf(std::floating_point auto v)
{
    return std::isinf(v);
}

BEE_FUNC BEE_CONSTEXPR bool IsInf(auto)
{
    return false;
}

BEE_FUNC BEE_CONSTEXPR bool IsFinite(std::floating_point auto v)
{
    return std::isfinite(v);
}

BEE_FUNC BEE_CONSTEXPR bool IsFinite(auto)
{
    return true;
}

template<cFloatType T> BEE_FUNC BEE_CONSTEXPR T FMA(T a, T b, T c)
{
    return glm::fma(a, b, c);
}

BEE_FUNC BEE_CONSTEXPR f32 ApproxSqrt(f32 x0)
{
    //BEE_DEBUG_ASSERT(x0 >= 0);

    union
    {
        i32 ix;
        f32 x;
    } u{};

    u.x  = x0;
    u.ix = 0x1fbb3f80 + (u.ix >> 1);
    u.x  = 0.5f * (u.x + x0 / u.x);
    u.x  = 0.5f * (u.x + x0 / u.x);
    return u.x;
}

BEE_FUNC BEE_CONSTEXPR f32 ApproxCbrt(f32 x0)
{
    //BEE_DEBUG_ASSERT(x0 >= 0);

    union
    {
        i32 ix;
        f32 x;
    } u{};

    u.x  = x0;
    u.ix = u.ix / 4 + u.ix / 16;
    u.ix = u.ix + u.ix / 16;
    u.ix = u.ix + u.ix / 256;
    u.ix = 0x2a5137a0 + u.ix;
    u.x  = 0.33333333f * (2.0f * u.x + x0 / (u.x * u.x));
    u.x  = 0.33333333f * (2.0f * u.x + x0 / (u.x * u.x));
    return u.x;
}

template<cFloatType T> BEE_FUNC BEE_CONSTEXPR T RecipSqrtFast(T x0)
{
    BEE_DEBUG_ASSERT(x0 > 0);

    if constexpr (sizeof(T) == 4) {
        union
        {
            i32 ix;
            f32 x;
        } u{};

        u.x       = x0;
        f32 xHalf = 0.5f * u.x;
        u.ix      = 0x5f37599e - (u.ix >> 1);
        u.x       = u.x * (1.5f - xHalf * u.x * u.x);
        u.x       = u.x * (1.5f - xHalf * u.x * u.x);
        return u.x;
    }
    else {
        union
        {
            i64 ix;
            f64 x;
        } u{};

        u.x  = x0;
        u.ix = 0x5fe6ec85e8000000LL - (u.ix >> 1);
        return u.x;
    }
}

BEE_FUNC BEE_CONSTEXPR u32 RoundUp(u32 x, u32 y)
{
    if (x == 0)
        return y;
    return ((x + y - 1) / y) * y;
}

BEE_FUNC BEE_CONSTEXPR Size AlignUp(Size value, Size alignment)
{
    // Assumes alignment is a power of two
    return (value + alignment - 1) & ~(alignment - 1);
}
} // namespace bee