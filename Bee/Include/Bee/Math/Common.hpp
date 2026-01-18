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
// ========== 基础算术 ==========

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

template <ArithType T>
constexpr auto Sqr(T x)
{
    return x * x;
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

template <typename T>
constexpr auto Clamp(T v, T min, T max) -> T requires std::is_arithmetic_v<T> || requires { v < min; }
{
    if (v < min)
        return min;
    if (v > max)
        return max;
    return v;
}

// ========== 舍入与根号 ==========

template <FloatType T>
constexpr auto Floor(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return floorf(x);
    else
        return floor(x);
    #else
    return std::floor(x);
    #endif
}

template <FloatType T>
constexpr auto Ceil(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return ceilf(x);
    else
        return ceil(x);
    #else
    return std::ceil(x);
    #endif
}

template <FloatType T>
constexpr auto Sqrt(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return sqrtf(x);
    else
        return sqrt(x);
    #else
    return std::sqrt(x);
    #endif
}

// ========== 三角与特殊函数 ==========

template <FloatType T>
constexpr auto Acos(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return acosf(x);
    else
        return acos(x);
    #else
    return std::acos(x);
    #endif
}

template <FloatType T>
constexpr auto Asin(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return asinf(x);
    else
        return asin(x);
    #else
    return std::asin(x);
    #endif
}

template <FloatType T>
constexpr auto Atan2(T y, T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return atan2f(y, x);
    else
        return atan2(y, x);
    #else
    return std::atan2(y, x);
    #endif
}

template <FloatType T>
constexpr auto Sin(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return sinf(x);
    else
        return sin(x);
    #else
    return std::sin(x);
    #endif
}

template <FloatType T>
constexpr auto Cos(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return cosf(x);
    else
        return cos(x);
    #else
    return std::cos(x);
    #endif
}

template <FloatType T>
constexpr auto Tan(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return tanf(x);
    else
        return tan(x);
    #else
    return std::tan(x);
    #endif
}

template <FloatType T>
constexpr auto Erf(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return erff(x);
    else
        return erf(x);
    #else
    return std::erf(x);
    #endif
}

template <ArithType T>
constexpr auto Sinc(T x)
{
    using F = MapFloatType<T>;

    auto fx = To<F>(x);
    if (Abs(fx) < MapFloatType<T>(kEpsilonF))
        return F(1);

    return std::sin(fx) / fx;
}

// ========== 安全三角函数 ==========

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

// ========== 角度与弧度 ==========

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

// ========== 基础数值运算 ==========

template <ArithType A, ArithType B, ArithType C>
constexpr auto FMA(A a, B b, C c)
{
    using F = CommonFloatType<A, B, C>;
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(F) == 4)
        return __fma_rn(To<F>(a), To<F>(b), To<F>(c));
    else
        return __dma_rn(To<F>(a), To<F>(b), To<F>(c));
    #else
    return std::fma(To<F>(a), To<F>(b), To<F>(c));
    #endif
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
    if constexpr (sizeof(F) == 4)
        return rhypotf(To<F>(x), To<F>(y));
    else
        return rhypot(To<F>(x), To<F>(y));
    #else
    return To<F>(1) / std::hypot(To<F>(x), To<F>(y));
    #endif
}

// ========== 区间与平滑 ==========

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

// ========== 插值 ==========

template <ArithType A, ArithType B, FloatType C>
constexpr auto Lerp(A a, B b, C x)
{
    using F = CommonFloatType<A, B, C>;

    auto fa = To<F>(a);
    auto fb = To<F>(b);
    auto fx = To<F>(x);
    return (To<F>(1) - fx) * fa + fx * fb;
}

// ========== 整数数学 ==========

template <ArithType A, ArithType B>
constexpr auto GCD(A x, B y)
{
    BEE_DCHECK(x != 0 && y != 0);
    return std::gcd(x, y);
}

template <ArithType A, ArithType B>
constexpr auto LCM(A x, B y)
{
    BEE_DCHECK(x != 0 && y != 0);
    return std::lcm(x, y);
}

template <ArithType T, ArithType... Ts>
constexpr auto GCD(T x, Ts... xs)
{
    ((x = GCD(x, xs)), ...);
    return x;
}

template <ArithType T, ArithType... Ts>
constexpr auto LCM(T x, Ts... xs)
{
    ((x = LCM(x, xs)), ...);
    return x;
}

// ========== 容差判断 ==========

template <FloatType F>
constexpr bool NearZero(F x, F eps = MapFloatType<F>(kDefaultEpsilonF))
{
    return Abs(x) < eps;
}

template <FloatType F>
constexpr bool NearOne(F x, F eps = MapFloatType<F>(kDefaultEpsilonF))
{
    return Abs(x - F(1)) < eps;
}

template <FloatType T>
constexpr auto AlmostZero(T x) noexcept
{
    return Abs(x) < MapFloatType<T>(kEpsilonF);
}

// ========== 浮点比较 ==========

template <ArithType T>
constexpr bool IsNear(T a, T b, T epsilon = T(1e-4)) noexcept
{
    return Abs(a - b) < epsilon;
}

template <ArithType T>
constexpr bool IsNearRel(T a, T b, T maxRelDiff = T(1e-4)) noexcept
{
    const T diff    = Abs(a - b);
    const T maxDiff = Max(Abs(a), Abs(b)) * maxRelDiff;
    return diff <= maxDiff;
}

// ========== 浮点分类 ==========

template <typename T>
constexpr auto IsFinite(T)
{
    return false;
}

template <FloatType T>
constexpr auto IsFinite(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return isfinitef(x);
    else
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

template <FloatType T>
constexpr auto IsInf(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return isinff(x);
    else
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

template <FloatType T>
constexpr auto IsNaN(T x)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return isnanf(x);
    else
        return isnan(x);
    #else
    return std::isnan(x);
    #endif
}

// ========== 浮点位级工具 ==========

template <FloatType T>
constexpr T CopySign(T x, T y) noexcept
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
        return __copysignf(x, y);
    else
        return __copysign(x, y);
    #else
    return std::copysign(x, y);
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

// ========== 相邻可表示浮点 ==========

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

// ========== 定向舍入算术 ==========

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
        return __dma_ru(a, b, c);
    }
    #else
    return NextFloatUp(std::fma(a, b, c));
    #endif
}

template <FloatType T>
constexpr T FMARoundDown(T a, T b, T c)
{
    #ifdef BEE_GPU_CODE
    if constexpr (sizeof(T) == 4)
    {
        return __fma_rd(a, b, c);
    }
    else
    {
        return __dma_rd(a, b, c);
    }
    #else
    return NextFloatDown(std::fma(a, b, c));
    #endif
}

// ========== 多项式 ==========

/// 使用 Horner 形式在点 t 上求值多项式：c0 + c1*t + c2*t^2 + ...
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

// ========== 求解二次方程 ==========

/// 求解一元二次方程 a*t^2 + b*t + c = 0，返回解的个数是否为 2（或线性退化时为 1）
/// 若返回 true，则保证 *t0 <= *t1
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

// ========== 快速近似 ==========

/// 快速 exp 近似（相对精度/误差不保证，适用于性能优先场景）
BEE_FUNC f32 FastExp(f32 x)
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
    bits     &= 0b10000000011111111111111111111111u;
    bits     |= (exponent + 127) << 23;
    return BitsToFloat(bits);
    #endif
}

/// 快速 exp 近似（相对精度/误差不保证，适用于性能优先场景）
BEE_FUNC f64 FastExp(f64 x)
{
    #ifdef BEE_GPU_CODE
    return exp(x);
    #else
    f64 xp  = x * 1.4426950408889634;
    f64 fxp = std::floor(xp), f = xp - fxp;
    int i   = To<int>(fxp);

    f64 twoToF   = EvaluatePolynomial(f, 1.0, 0.695556856, 0.226173572, 0.0781455737);
    int exponent = Exponent(twoToF) + i;
    if (exponent < -1022)
        return 0;
    if (exponent > 1023)
        return kInfinityD;

    u64 bits = FloatToBits(twoToF);
    bits     &= 0b1000000000001111111111111111111111111111111111111111111111111111ull;
    bits     |= (u64)(exponent + 1023) << 52;
    return BitsToFloat(bits);
    #endif
}

/// 快速 sqrt 近似（要求 x0 >= 0）
constexpr f32 FastSqrt(f32 x0)
{
    BEE_CHECK(x0 >= 0);

    i32 ix = BitCast<i32>(x0);
    ix     = 0x1fbb3f80 + (ix >> 1);
    f32 x  = BitCast<f32>(ix);
    x      = 0.5f * (x + x0 / x);
    x      = 0.5f * (x + x0 / x);
    return x;
}

/// 快速 cbrt 近似（要求 x0 >= 0）
constexpr f32 FastCbrt(f32 x0)
{
    BEE_CHECK(x0 >= 0);

    i32 ix = BitCast<i32>(x0);
    ix     = ix / 4 + ix / 16;
    ix     = ix + ix / 16;
    ix     = ix + ix / 256;
    ix     = 0x2a5137a0 + ix;
    f32 x  = BitCast<f32>(ix);
    x      = 0.33333333f * (2.0f * x + x0 / (x * x));
    x      = 0.33333333f * (2.0f * x + x0 / (x * x));
    return x;
}

/// 快速 1/sqrt 近似（要求 x0 > 0）
template <FloatType T>
constexpr T FastInvSqrt(T x0)
{
    BEE_CHECK(x0 > 0);

    if constexpr (sizeof(T) == 4)
    {
        i32 ix    = BitCast<i32>(static_cast<f32>(x0));
        f32 xHalf = 0.5f * x0;
        ix        = 0x5f37599e - (ix >> 1);
        f32 x     = BitCast<f32>(ix);
        x         = x * (1.5f - xHalf * x * x);
        x         = x * (1.5f - xHalf * x * x);
        return x;
    }
    else
    {
        i64 ix    = BitCast<i64>(static_cast<f64>(x0));
        f64 xHalf = 0.5 * x0;
        ix        = 0x5fe6ec85e8000000LL - (ix >> 1);
        f64 x     = BitCast<f64>(ix);
        x         = x * (1.5 - xHalf * x * x);
        x         = x * (1.5 - xHalf * x * x);
        return x;
    }
}

// ========== 概率与统计 ==========

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

// ========== 数值健壮性检查 ==========

/// 检查值是否在有效范围内（非 NaN，非 Inf）
template <std::floating_point T>
constexpr bool IsValidValue(T x) noexcept
{
    return !IsNaN(x) && !IsInf(x);
}

/// 检查值是否在指定范围内，包含边界宽容
template <std::floating_point T>
constexpr bool IsInRange(T value, T min, T max, T tolerance = T(1e-6)) noexcept
{
    return value >= (min - tolerance) && value <= (max + tolerance);
}

/// 检查是否为安全的非零值, 避免下溢
template <std::floating_point T>
constexpr bool IsSafeNonZero(T x, T threshold = To<T>(kDefaultEpsilonF)) noexcept
{
    return Abs(x) >= threshold;
}

template <std::floating_point T>
constexpr auto SafeDivide(T numerator, T denominator, T fallback = T{}) noexcept -> std::optional<T>
{
    if (!IsSafeNonZero(denominator))
        return fallback;
    return numerator / denominator;
}

template <typename T>
constexpr bool IsSafeLengthSqr(T lenSq, T epsilonSq = T(1e-10)) noexcept
{
    return lenSq >= epsilonSq && !IsNaN(lenSq) && !IsInf(lenSq);
}

} // namespace bee
