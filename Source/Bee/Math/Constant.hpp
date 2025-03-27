/**
 * @File Constant.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Math/Math.hpp"

namespace bee {

template<ArithmeticType T>
BEE_FUNC BEE_CONSTEXPR T cast_to(ArithmeticType auto f) noexcept
    requires std::is_nothrow_convertible_v<decltype(f), T>
{
    return static_cast<T>(f);
}

// clang-format off
static constexpr auto kHalf      = cast_to<f32>(0.5);
static constexpr auto kZero      = cast_to<f32>(0);
static constexpr auto kOne       = cast_to<f32>(1);
static constexpr auto kTwo       = cast_to<f32>(2);

static constexpr auto kPi        = cast_to<f32>(3.141592653589793238462643383279502884197);
static constexpr auto kTwoPi     = cast_to<f32>(kPi * 2);
static constexpr auto kHalfPi    = cast_to<f32>(kPi / 2);
static constexpr auto kQuarterPi = cast_to<f32>(kPi / 4);
static constexpr auto kSqrtPi    = cast_to<f32>(1.772453850905516027);
static constexpr auto kSqrt2Pi   = cast_to<f32>(2.506628274631000502);

static constexpr auto kInvPi     = cast_to<f32>(1.0 / kPi);
static constexpr auto kInv2Pi    = cast_to<f32>(kInvPi / 2);
static constexpr auto kInv4Pi    = cast_to<f32>(kInvPi / 4);
static constexpr auto kInvSqrtPi = cast_to<f32>(1.0 / kSqrtPi);
static constexpr auto kInv3      = cast_to<f32>(0.3333333333333333333333333333333333333333);

static constexpr auto kSqrt2     = cast_to<f32>(1.4142135623730950488016887242096980785696718753769);
static constexpr auto kSqrt3     = cast_to<f32>(1.73205080756887729352744634150587236694280525381038);
static constexpr auto kSqrt5     = cast_to<f32>(2.2360679774997896964091736687312762354406183596115);

static constexpr auto kGoldenRatio = cast_to<f32>(1.61803398874989484820458683436563811);

static constexpr auto kInfinity              = std::numeric_limits<f32>::infinity();
static constexpr auto kMachineEpsilon        = cast_to<f32>(std::numeric_limits<f32>::epsilon() * cast_to<f32>(0.5));
static constexpr auto kShadowEpsilon         = cast_to<f32>(0.000001);
static constexpr auto kFloatMin              = std::numeric_limits<f32>::min();
static constexpr auto kFloatMax              = std::numeric_limits<f32>::max();
static constexpr auto kFloatOneMinusEpsilon  = cast_to<f32>(0x1.fffffep-1);
static constexpr auto kDoubleOneMinusEpsilon = cast_to<f64>(0x1.fffffffffffffp-1);
static constexpr auto kOneMinusEpsilon       = cast_to<f32>(kDoubleOneMinusEpsilon);
// clang-format on

#define DEFINE_CONSTANT_FUNC(name, val)                                                                                                              \
    template<FloatType F = f32> BEE_FUNC constexpr F name() noexcept                                                                                 \
    {                                                                                                                                                \
        return cast_to<F>(val);                                                                                                                      \
    }

DEFINE_CONSTANT_FUNC(Pi, kPi)
DEFINE_CONSTANT_FUNC(TwoPi, kTwoPi)
DEFINE_CONSTANT_FUNC(HalfPi, kHalfPi)
DEFINE_CONSTANT_FUNC(QuarterPi, kQuarterPi)
DEFINE_CONSTANT_FUNC(SqrtPi, kSqrtPi)
DEFINE_CONSTANT_FUNC(Sqrt2Pi, kSqrt2Pi)

DEFINE_CONSTANT_FUNC(InvPi, kInvPi)
DEFINE_CONSTANT_FUNC(Inv2Pi, kInv2Pi)
DEFINE_CONSTANT_FUNC(Inv4Pi, kInv4Pi)
DEFINE_CONSTANT_FUNC(InvSqrtPi, kInvSqrtPi)
DEFINE_CONSTANT_FUNC(Inv3, kInv3)

DEFINE_CONSTANT_FUNC(Sqrt2, kSqrt2)
DEFINE_CONSTANT_FUNC(Sqrt3, kSqrt3)
DEFINE_CONSTANT_FUNC(Sqrt5, kSqrt5)

#undef DEFINE_CONSTANT_FUNC

template<FloatType T> BEE_FUNC constexpr T Epsilon() noexcept
{
    return std::numeric_limits<T>::epsilon();
}

template<FloatType T> BEE_FUNC constexpr T Infinity() noexcept
{
    return std::numeric_limits<T>::infinity();
}

template<ArithmeticType T> BEE_FUNC constexpr T Zero() noexcept
{
    return T(0);
}

template<FloatType T> BEE_FUNC constexpr T Half() noexcept
{
    return T(0.5);
}

template<ArithmeticType T> BEE_FUNC constexpr T One() noexcept
{
    return T(1);
}

template<ArithmeticType T> BEE_FUNC constexpr T Two() noexcept
{
    return T(2);
}

template<FloatType T> BEE_FUNC constexpr auto Radians(T x) noexcept
{
    return x * decltype(x)(0.01745329251994329576923690768489);
}

template<FloatType T> BEE_FUNC constexpr auto Degrees(T x) noexcept
{
    return x * decltype(x)(57.295779513082320876798154814105);
}

template<i32 N> BEE_FUNC constexpr auto Pow(std::floating_point auto v) noexcept
{
    if constexpr (N == 0)
        return 1;
    else if constexpr (N == 1)
        return v;
    else if constexpr (N < 0)
        return 1 / Pow<-N>(v);
    else {
        auto v_2 = Pow<N / 2>(v);
        return v_2 * v_2 * Pow<N & 1>(v);
    }
}

BEE_FUNC constexpr auto Pow2(std::floating_point auto v) noexcept
{
    return Pow<2>(v);
}

} // namespace bee