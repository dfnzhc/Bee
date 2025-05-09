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

BEE_DEFINE_COMMON_FUNC(FloatType, Floor, floor)
BEE_DEFINE_COMMON_FUNC(FloatType, Ceil, ceil)
BEE_DEFINE_COMMON_FUNC(FloatType, Trunc, trunc)
BEE_DEFINE_COMMON_FUNC(FloatType, Round, round)
BEE_DEFINE_COMMON_FUNC(FloatType, Sqrt, sqrt)
BEE_DEFINE_COMMON_FUNC(FloatType, Exp, exp)
BEE_DEFINE_COMMON_FUNC(FloatType, Exp2, exp2)
BEE_DEFINE_COMMON_FUNC(FloatType, Log, log)
BEE_DEFINE_COMMON_FUNC(FloatType, Log2, log2)
BEE_DEFINE_COMMON_FUNC(FloatType, Sin, sin)
BEE_DEFINE_COMMON_FUNC(FloatType, Cos, cos)
BEE_DEFINE_COMMON_FUNC(FloatType, Tan, tan)
BEE_DEFINE_COMMON_FUNC(FloatType, Sinh, sinh)
BEE_DEFINE_COMMON_FUNC(FloatType, Cosh, cosh)
BEE_DEFINE_COMMON_FUNC(FloatType, Tanh, tanh)
BEE_DEFINE_COMMON_FUNC(FloatType, ASin, asin)
BEE_DEFINE_COMMON_FUNC(FloatType, ACos, acos)
BEE_DEFINE_COMMON_FUNC(FloatType, ATan, atan)
BEE_DEFINE_COMMON_FUNC(FloatType, ASinh, asinh)
BEE_DEFINE_COMMON_FUNC(FloatType, ACosh, acosh)
BEE_DEFINE_COMMON_FUNC(FloatType, ATanh, atanh)

#undef BEE_DEFINE_COMMON_FUNC

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

template<FloatType T> BEE_FUNC BEE_CONSTEXPR T RecipSqrtFast(T x0)
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

BEE_FUNC BEE_CONSTEXPR u32 Parity(u32 x)
{
    x ^= x >> 1;
    x ^= x >> 2;
    x ^= x >> 4;
    x ^= x >> 8;
    x ^= x >> 16;
    return x & 1;
}

BEE_FUNC BEE_CONSTEXPR u32 NextPowerOfTwo(u32 x)
{
    if (x == 0)
        return 0;

    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return ++x;
}

BEE_FUNC BEE_CONSTEXPR u32 PreviousPowerOfTwo(u32 x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x - (x >> 1);
}

BEE_FUNC BEE_CONSTEXPR u32 ClosestPowerOfTwo(u32 x)
{
    auto nx = NextPowerOfTwo(x);
    auto px = PreviousPowerOfTwo(x);
    return (nx - x) > (x - px) ? px : nx;
}

template<IntegralType I>
BEE_FUNC BEE_CONSTEXPR bool IsPowerOfTwo(I x)
{
    return (x > 0) && ((x & (x - 1)) == 0);
}

template<std::unsigned_integral T> BEE_FUNC BEE_CONSTEXPR T BitSwap(T x)
{
    if constexpr (sizeof(T) == 1) {
        return x;
    }
    else if constexpr (sizeof(T) == 2) {
        return (x >> 8) | (x << 8);
    }
    else if constexpr (sizeof(T) == 4) {
        return ((x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24));
    }
    else {
        x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
        x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
        x = (x & 0x00FF00FF00FF00FF) << 8 | (x & 0xFF00FF00FF00FF00) >> 8;
        return x;
    }
}
} // namespace bee