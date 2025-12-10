/**
 * @File Common.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <numeric>

#include "Constants.hpp"
#include "Bits.hpp"
#include "Bee/Core/Check.hpp"
#include "Bee/Core/Concepts.hpp"

namespace bee
{

// -------------------- 
// 基础函数

template <ArithType T>
constexpr T Abs(T x)
{
    if constexpr (std::is_unsigned_v<T>)
    {
        return x;
    }
    else
    {
        return x < 0 ? -x : x;
    }
}

template <ArithType A, ArithType B>
constexpr auto Min(A x, B y)
{
    using R = std::common_type_t<A, B>;
    return std::min(To<R>(x), To<R>(y));
}

template <ArithType A, ArithType B>
constexpr auto Max(A x, B y)
{
    using R = std::common_type_t<A, B>;
    return std::max(To<R>(x), To<R>(y));
}

template <ArithType T, ArithType U, ArithType... Ts>
    requires ConvertibleTo<U, T> && AllConvertibleTo<T, Ts...>
constexpr auto Min(T a, U b, Ts... vals) noexcept
{
    using R = std::common_type_t<T, U>;

    const auto m = To<R>(a < b ? a : b);
    if constexpr (sizeof...(vals) > 0)
    {
        return Min(m, To<R>(vals)...);
    }
    return m;
}

template <ArithType T, ArithType U, ArithType... Ts>
    requires ConvertibleTo<U, T> && AllConvertibleTo<T, Ts...>
constexpr auto Max(T a, U b, Ts... vals) noexcept
{
    using R = std::common_type_t<T, U>;

    const auto m = To<R>(a > b ? a : b);
    if constexpr (sizeof...(vals) > 0)
    {
        return Max(m, To<R>(vals)...);
    }
    return m;
}

template <ArithType T>
constexpr auto Sqr(T x)
{
    return x * x;
}

template <IntegralType T>
constexpr T Mod(T a, T b) noexcept
{
    T result = a - (a / b) * b;
    return To<T>((result < 0) ? result + b : result);
}

template <ArithType T>
constexpr auto InvSqrt(T x)
{
    using F = MapFloatType<T>;
    return To<F>(1) / std::sqrt(To<F>(x));
}

template <ArithType A, ArithType B>
constexpr auto InvHypot(A x, B y)
{
    using F = CommonFloatType<A, B>;

    #ifdef BEE_GPU_CODE
    return ::rhypot(To<F>(x), To<F>(y));
    #else
    return To<F>(1) / std::hypot(To<F>(x), To<F>(y));
    #endif
}

template <ArithType T, FloatType F = f32>
constexpr bool NearZero(T x, F eps = To<F>((kEpsilonF + kEpsilonD) * 0.5))
{
    using R = MapFloatType<T>;
    return std::abs(To<R>(x)) < eps;
}

template <ArithType T, FloatType F = f32>
constexpr bool NearOne(T x, F eps = To<F>((kEpsilonF + kEpsilonD) * 0.5))
{
    using R = MapFloatType<T>;
    return NearZero(To<R>(x) - To<R>(1), eps);
}

template <typename T>
constexpr auto IsFinite(T)
{
    return false;
}

constexpr auto IsFinite(FloatType auto x)
{
    #ifdef BEE_GPU_CODE
    return isfinite(x);
    #else
    return std::isfinite(x);
    #endif
}

template <typename T>
constexpr auto IsInf(T)
{
    return false;
}

constexpr auto IsInf(FloatType auto x)
{
    #ifdef BEE_GPU_CODE
    return isinf(x);
    #else
    return std::isinf(x);
    #endif
}

template <typename T>
constexpr auto IsNaN(T)
{
    return false;
}

constexpr auto IsNaN(FloatType auto x)
{
    #ifdef BEE_GPU_CODE
    return isnan(x);
    #else
    return std::isnan(x);
    #endif
}

template <FloatType T>
constexpr auto FloatToBits(T x) -> std::conditional_t<sizeof(T) == 4u, u32, u64>
{
    if constexpr (sizeof(T) == 4)
    {
        #ifdef BEE_GPU_CODE
        return __float_as_uint(x);
        #else
        return BitCast<u32>(x);
        #endif
    }
    else
    {
        #ifdef BEE_GPU_CODE
        return __double_as_longlong(x);
        #else
        return BitCast<u64>(x);
        #endif
    }
}

template <UnsignedType T>
constexpr auto BitsToFloat(T x) -> std::conditional_t<sizeof(T) == 4u, f32, f64>
{
    if constexpr (sizeof(T) == 4)
    {
        #ifdef BEE_GPU_CODE
        return __uint_as_float(x);
        #else
        return BitCast<f32>(x);
        #endif
    }
    else
    {
        #ifdef BEE_GPU_CODE
        return __longlong_as_double(x);
        #else
        return BitCast<f64>(x);
        #endif
    }
}

template <FloatType T>
constexpr int Exponent(T x) noexcept
{
    if constexpr (sizeof(T) == 4)
    {
        return ((FloatToBits(x) >> 23) & 0xFF) - 127;
    }
    else
    {
        return ((FloatToBits(x) >> 52) & 0x7FF) - 1023;
    }
}

template <FloatType T>
constexpr auto Significand(T x) noexcept -> std::conditional_t<sizeof(T) == 4u, u32, u64>
{
    if constexpr (sizeof(T) == 4)
    {
        return FloatToBits(x) & ((1 << 23) - 1);
    }
    else
    {
        return FloatToBits(x) & ((1ull << 52) - 1);
    }
}

template <FloatType T>
constexpr auto SignBit(T x) noexcept -> std::conditional_t<sizeof(T) == 4u, u32, u64>
{
    if constexpr (sizeof(T) == 4)
    {
        return FloatToBits(x) & 0x80000000;
    }
    else
    {
        return FloatToBits(x) & 0x8000000000000000;
    }
}

template <FloatType T>
constexpr T NextFloatUp(T x) noexcept
{
    if (IsInf(x) && x > T(0))
        return x;

    if (x == -T(0))
        x = T(0);

    auto ui = FloatToBits(x);
    if (x >= 0)
        ++ui;
    else
        --ui;
    return BitsToFloat(ui);
}

template <FloatType T>
constexpr T NextFloatDown(T x) noexcept
{
    if (IsInf(x) && x < T(0))
        return x;

    if (x == T(0))
        x = -T(0);

    auto ui = FloatToBits(x);
    if (x > 0)
        --ui;
    else
        ++ui;
    return BitsToFloat(ui);
}

template <FloatType T>
constexpr T AddRoundUp(T a, T b)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fadd_ru(a, b);
    }
    else
    {
        return __dadd_ru(a, b);
    }
    #else
    return NextFloatUp(a + b);
    #endif
}

template <FloatType T>
constexpr T AddRoundDown(T a, T b)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fadd_rd(a, b);
    }
    else
    {
        return __dadd_rd(a, b);
    }
    #else
    return NextFloatDown(a + b);
    #endif
}

template <FloatType T>
constexpr T SubRoundUp(T a, T b)
{
    return AddRoundUp(a, -b);
}

template <FloatType T>
constexpr T SubRoundDown(T a, T b)
{
    return AddRoundDown(a, -b);
}

template <FloatType T>
constexpr T MulRoundUp(T a, T b)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fmul_ru(a, b);
    }
    else
    {
        return __dmul_ru(a, b);
    }
    #else
    return NextFloatUp(a * b);
    #endif
}

template <FloatType T>
constexpr T MulRoundDown(T a, T b)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fmul_rd(a, b);
    }
    else
    {
        return __dmul_rd(a, b);
    }
    #else
    return NextFloatDown(a * b);
    #endif
}

template <FloatType T>
constexpr T DivRoundUp(T a, T b)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fdiv_ru(a, b);
    }
    else
    {
        return __ddiv_ru(a, b);
    }
    #else
    return NextFloatUp(a / b);
    #endif
}

template <FloatType T>
constexpr T DivRoundDown(T a, T b)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fdiv_rd(a, b);
    }
    else
    {
        return __ddiv_rd(a, b);
    }
    #else
    return NextFloatDown(a / b);
    #endif
}

template <FloatType T>
constexpr T SqrtRoundUp(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fsqrt_ru(x);
    }
    else
    {
        return __dsqrt_ru(x);
    }
    #else
    return NextFloatUp(std::sqrt(x));
    #endif
}

template <FloatType T>
constexpr T SqrtRoundDown(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fsqrt_rd(x);
    }
    else
    {
        return __dsqrt_rd(x);
    }
    #else
    return NextFloatDown(std::sqrt(x));
    #endif
}

template <FloatType T>
constexpr T FMARoundUp(T a, T b, T c)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fma_ru(a, b, c);
    }
    else
    {
        return __fma_ru(a, b, c);
    }
    #else
    return NextFloatUp(FMA(a, b, c));
    #endif
}

template <FloatType T>
constexpr T FMARoundDown(T a, T b, T c)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fma_ru(a, b, c);
    }
    else
    {
        return __fma_ru(a, b, c);
    }
    #else
    return NextFloatDown(FMA(a, b, c));
    #endif
}

template <ArithType A>
constexpr auto Saturate(A x)
{
    return std::clamp(x, To<A>(0), To<A>(1));
}

template <FloatType T>
constexpr T SmoothStep(T t)
{
    t = std::clamp(t, T(0), T(1));
    return t * t * (T(3) - T(2) * t);
}

template <FloatType T>
constexpr T SmootherStep(T t)
{
    t = std::clamp(t, T(0), T(1));
    return t * t * t * (t * (t * T(6) - T(15)) + T(10));
}

template <ArithType T>
constexpr auto SafeArcSin(T x)
{
    BEE_DCHECK(x >= To<T>(-1.0001) && x <= To<T>(1.0001));
    return std::asin(std::clamp(x, To<T>(-1), To<T>(1)));
}

template <ArithType T>
constexpr auto SafeArcCos(T x)
{
    BEE_DCHECK(x >= To<T>(-1.0001) && x <= To<T>(1.0001));
    return std::acos(std::clamp(x, To<T>(-1), To<T>(1)));
}

// -------------------- 
// 其他方法

template <FloatType T>
constexpr auto AlmostZero(T x) noexcept
{
    return T(1) - x * x == T(1);
}

template <ArithType T>
constexpr auto Sinc(T x)
{
    using F = MapFloatType<T>;

    auto fx = To<F>(x);
    if (AlmostZero(fx))
        return F(1);

    return std::sin(fx) / fx;
}

template <ArithType T>
constexpr auto Degree2Radian(T degree) noexcept
{
    using F = MapFloatType<T>;
    return To<F>(Pi<F>() / 180) * To<F>(degree);
}

template <ArithType T>
constexpr auto Radian2Degree(T radian) noexcept
{
    using F = MapFloatType<T>;
    return To<F>(InvPi<F>() * 180) * To<F>(radian);
}

template <ArithType A, ArithType B>
constexpr auto GCD(A x, B y)
{
    return std::gcd(x, y);
}

template <ArithType A, ArithType B>
constexpr auto LCM(A x, B y)
{
    return std::lcm(x, y);
}

constexpr auto GCD(auto x, auto... xs)
{
    return ((x = GCD(x, xs)), ...);
}

constexpr auto LCM(auto x, auto... xs)
{
    return ((x = LCM(x, xs)), ...);
}

template <ArithType A, ArithType B, FloatType C>
constexpr auto Lerp(A a, B b, C x)
{
    using F = CommonFloatType<A, B, C>;

    auto fa = To<F>(a);
    auto fb = To<F>(b);
    auto fx = To<F>(x);
    return (To<F>(1) - fx) * fa + fx * fb;
}

template <ArithType T, ArithType C>
constexpr T EvaluatePolynomial(T, C c) noexcept
{
    return To<T>(c);
}

template <ArithType T, ArithType C, ArithType... Args>
constexpr T EvaluatePolynomial(T t, C c, Args... cRemaining)
{
    return std::fma(t, To<T>(EvaluatePolynomial(t, cRemaining...)), To<T>(c));
}

template <FloatType T>
constexpr bool Quadratic(T a, T b, T c, T* t0, T* t1)
{
    if (a == 0)
    {
        if (b == 0)
            return false;

        *t0 = *t1 = -c / b;
        return true;
    }

    auto discriminant = b * b - To<T>(4) * a * c;
    if (discriminant < 0)
        return false;
    auto rootDiscriminant = std::sqrt(discriminant);

    auto q = -To<T>(0.5) * (b + std::copysign(rootDiscriminant, b));
    *t0    = q / a;
    *t1    = c / q;

    if (*t0 > *t1)
        std::swap(*t0, *t1);

    return true;
}

constexpr f32 FastExp(f32 x)
{
    #ifdef BEE_GPU_CODE
    return __expf(x);
    #else
    f32 xp  = x * 1.442695041f;
    f32 fxp = std::floor(xp), f = xp - fxp;
    int i   = To<int>(fxp);

    f32 twoToF   = EvaluatePolynomial(f, 1.f, 0.695556856f, 0.226173572f, 0.0781455737f);
    int exponent = Exponent(twoToF) + i;
    if (exponent < -126)
        return 0;
    if (exponent > 127)
        return kInfinityF;

    u32 bits = FloatToBits(twoToF);
    bits &= 0b10000000011111111111111111111111u;
    bits |= (exponent + 127) << 23;
    return BitsToFloat(bits);
    #endif
}

constexpr f32 FastSqrt(f32 x0)
{
    BEE_CHECK(x0 >= 0);

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

constexpr f32 FastCbrt(f32 x0)
{
    BEE_CHECK(x0 >= 0);

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

template <FloatType T>
constexpr T FastInvSqrt(T x0)
{
    BEE_CHECK(x0 > 0);

    if constexpr (sizeof(T) == 4)
    {
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
    else
    {
        union
        {
            i64 ix;
            f64 x;
        } u{};

        u.x       = x0;
        f64 xHalf = 0.5 * u.x;
        u.ix      = 0x5fe6ec85e8000000LL - (u.ix >> 1);
        u.x       = u.x * (1.5 - xHalf * u.x * u.x);
        u.x       = u.x * (1.5 - xHalf * u.x * u.x);
        return u.x;
    }
}

template <FloatType T>
constexpr T Gaussian(T x, T mu = 0, T sigma = 1)
{
    return To<T>(1) / std::sqrt(TwoPi<T>() * sigma * sigma) * FastExp(-Sqr(x - mu) / (To<T>(2) * sigma * sigma));
}

template <FloatType T>
constexpr T GaussianIntegral(T x0, T x1, T mu = 0, T sigma = 1)
{
    T sigmaRoot2 = sigma * Sqrt2<T>();
    return To<T>(0.5) * (std::erf((mu - x0) / sigmaRoot2) - std::erf((mu - x1) / sigmaRoot2));
}

} // namespace bee
