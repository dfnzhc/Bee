/**
 * @File Common.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include "./Setup.hpp"
#include "./Constant.hpp"

namespace bee {

#define XIHE_DEFINE_COMMON_FUNC(Type, Name, Func)                                                                                                    \
    template<Type T> BEE_FUNC BEE_CONSTEXPR T Name(T x) noexcept                                                                                     \
    {                                                                                                                                                \
        return glm::Func(x);                                                                                                                         \
    }

XIHE_DEFINE_COMMON_FUNC(FloatType, Floor, floor)
XIHE_DEFINE_COMMON_FUNC(FloatType, Ceil, ceil)
XIHE_DEFINE_COMMON_FUNC(FloatType, Trunc, trunc)
XIHE_DEFINE_COMMON_FUNC(FloatType, Round, round)
XIHE_DEFINE_COMMON_FUNC(FloatType, Sqrt, sqrt)
XIHE_DEFINE_COMMON_FUNC(FloatType, Exp, exp)
XIHE_DEFINE_COMMON_FUNC(FloatType, Exp2, exp2)
XIHE_DEFINE_COMMON_FUNC(FloatType, Log, log)
XIHE_DEFINE_COMMON_FUNC(FloatType, Log2, log2)
XIHE_DEFINE_COMMON_FUNC(FloatType, Sin, sin)
XIHE_DEFINE_COMMON_FUNC(FloatType, Cos, cos)
XIHE_DEFINE_COMMON_FUNC(FloatType, Tan, tan)
XIHE_DEFINE_COMMON_FUNC(FloatType, Sinh, sinh)
XIHE_DEFINE_COMMON_FUNC(FloatType, Cosh, cosh)
XIHE_DEFINE_COMMON_FUNC(FloatType, Tanh, tanh)
XIHE_DEFINE_COMMON_FUNC(FloatType, ASin, asin)
XIHE_DEFINE_COMMON_FUNC(FloatType, ACos, acos)
XIHE_DEFINE_COMMON_FUNC(FloatType, ATan, atan)
XIHE_DEFINE_COMMON_FUNC(FloatType, ASinh, asinh)
XIHE_DEFINE_COMMON_FUNC(FloatType, ACosh, acosh)
XIHE_DEFINE_COMMON_FUNC(FloatType, ATanh, atanh)

#undef XIHE_DEFINE_COMMON_FUNC

template<FloatType T> BEE_INLINE BEE_CONSTEXPR T Log10(T x) noexcept
{
    return std::log10(x);
}

template<FloatType T> BEE_INLINE BEE_CONSTEXPR T ATan2(T y, T x) noexcept
{
    return std::atan2(y, x);
}

template<SignedType T> BEE_FUNC BEE_CONSTEXPR T Abs(T x) noexcept
{
    return glm::abs(x);
}

template<SignedType T> BEE_FUNC BEE_CONSTEXPR T Sign(T x) noexcept
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

template<ArithmeticType T> BEE_FUNC BEE_CONSTEXPR bool Equal(T x, T y) noexcept
{
    return x == y;
}

template<ArithmeticType T> BEE_FUNC BEE_CONSTEXPR bool NotEqual(T x, T y) noexcept
{
    return not Equal(x, y);
}

template<FloatType T> BEE_FUNC BEE_CONSTEXPR bool Approx(T x, T y, f32 eps = kShadowEpsilon) noexcept
{
    return Abs(x - y) < eps;
}

template<FloatType T> BEE_FUNC BEE_CONSTEXPR bool NotApprox(T x, T y, f32 eps = kShadowEpsilon) noexcept
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

template<FloatType T> BEE_FUNC BEE_CONSTEXPR T FMA(T a, T b, T c)
{
    return glm::fma(a, b, c);
}

BEE_FUNC BEE_CONSTEXPR f32 ApproxSqrt(f32 x0)
{
    BEE_ASSERT(x0 >= 0);

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
    BEE_ASSERT(x0 >= 0);

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

template<FloatType T> BEE_FUNC BEE_CONSTEXPR T RecipSqrtFast(T x0)
{
    XIHE_ASSERT(x0 > 0);

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
    //    return (x + (y - 1)) & ~(y - 1);
}

BEE_FUNC BEE_CONSTEXPR u32 Parity(u32 x)
{
    x ^= x >> 1;
    x ^= x >> 2;
    x ^= x >> 4;
    x ^= x >> 8;
    x ^= x >> 16;
    return x & 1;
}

} // namespace bee