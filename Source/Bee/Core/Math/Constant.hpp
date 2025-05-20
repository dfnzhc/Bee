/**
 * @File Constant.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Math/Math.hpp"

namespace bee {

// clang-format off
constexpr f64 kPi =      3.14159265358979323846264338327950288;
constexpr f64 kTwoPi =   6.28318530717958647692528676655900577;
constexpr f64 kPiOver2 = 1.57079632679489661923132169163975144;
constexpr f64 kPiOver3 = 1.04719755119659774615421446109316763;
constexpr f64 kPiOver4 = 0.785398163397448309615660845819875721;
constexpr f64 kPiOver6 = 0.52359877559829887307710723054658381;
constexpr f64 kPiOver8 = 0.39269908169872415480783042290993786;
constexpr f64 kSqrtPi =  1.772453850905516027;
constexpr f64 kSqrt2Pi = 2.506628274631000502;

constexpr f64 kInvPi =        0.318309886183790671537767526745028724;
constexpr f64 kInv2Pi =       kInvPi / 2;
constexpr f64 kInv4Pi =       kInvPi / 4;
constexpr f64 kInvSqrtPi =    0.564189583547756286948079451560772586;
constexpr f64 kInvSqrtTwoPi = 0.398942280401432677939946059934381868;

constexpr f64 kE =      2.71828182845904523536028747135266250;
constexpr f64 kEgamma = 0.577215664901532860606512090082402431;    ///< The Euler-Mascheroni constant
constexpr f64 kPhi =    1.618033988749894848204586834365638118;    ///< The golden ratio
constexpr f64 kLog2E =  1.4426950408889634;
constexpr f64 kLog10E = 0.4342944819032518;
constexpr f64 kLn2 =    0.693147180559945309417232121458176568;
constexpr f64 kLn3 =    1.09861228866810969139524523692;
constexpr f64 kLn5 =    1.60943791243410037460075933323;
constexpr f64 kLn10 =   2.30258509299404568401799145468436421;

constexpr f64 kSqrt2 = 1.4142135623730950488016887242096980785696718753769;
constexpr f64 kSqrt3 = 1.73205080756887729352744634150587236694280525381038;
constexpr f64 kSqrt5 = 2.2360679774997896964091736687312762354406183596115;

constexpr f64 kInvSqrt2 = 0.707106781186547524400844362104849039;
constexpr f64 kInvSqrt3 = 0.577350269189625764509148780501957456;

constexpr f32  kEpsilonF = 1e-6f;
constexpr f64  kEpsilonD = 1e-12;
constexpr Float kEpsilon   = std::is_same_v<Float, f32> ? kEpsilonF : kEpsilonD;

// clang-format on

#define DEFINE_CONSTANT_FUNC(name)                                                                                                              \
    template<cFloatType F = f32> BEE_FUNC constexpr F name() noexcept                                                                                 \
    {                                                                                                                                                \
        return cast_to<F>(k##name);                                                                                                                      \
    }

DEFINE_CONSTANT_FUNC(Pi);
DEFINE_CONSTANT_FUNC(TwoPi);
DEFINE_CONSTANT_FUNC(PiOver2);
DEFINE_CONSTANT_FUNC(PiOver3);
DEFINE_CONSTANT_FUNC(PiOver4);
DEFINE_CONSTANT_FUNC(PiOver6);
DEFINE_CONSTANT_FUNC(PiOver8);
DEFINE_CONSTANT_FUNC(SqrtPi);
DEFINE_CONSTANT_FUNC(Sqrt2Pi);

DEFINE_CONSTANT_FUNC(InvPi);
DEFINE_CONSTANT_FUNC(Inv2Pi);
DEFINE_CONSTANT_FUNC(Inv4Pi);
DEFINE_CONSTANT_FUNC(InvSqrtPi);
DEFINE_CONSTANT_FUNC(InvSqrtTwoPi);

DEFINE_CONSTANT_FUNC(E);
DEFINE_CONSTANT_FUNC(Egamma);
DEFINE_CONSTANT_FUNC(Phi);
DEFINE_CONSTANT_FUNC(Log2E);
DEFINE_CONSTANT_FUNC(Log10E);
DEFINE_CONSTANT_FUNC(Ln2);
DEFINE_CONSTANT_FUNC(Ln3);
DEFINE_CONSTANT_FUNC(Ln5);
DEFINE_CONSTANT_FUNC(Ln10);

DEFINE_CONSTANT_FUNC(Sqrt2);
DEFINE_CONSTANT_FUNC(Sqrt3);
DEFINE_CONSTANT_FUNC(Sqrt5);

DEFINE_CONSTANT_FUNC(InvSqrt2);
DEFINE_CONSTANT_FUNC(InvSqrt3);

#undef DEFINE_CONSTANT_FUNC

template<cFloatType T> BEE_FUNC constexpr T MachineEpsilon() noexcept
{
    return numeric_limits<T>::epsilon();
}

template<cFloatType T> BEE_FUNC constexpr T Infinity() noexcept
{
    return numeric_limits<T>::infinity();
}

template<cArithmeticType T> BEE_FUNC constexpr T Zero() noexcept
{
    return T(0);
}

template<cFloatType T> BEE_FUNC constexpr T Half() noexcept
{
    return T(0.5);
}

template<cArithmeticType T> BEE_FUNC constexpr T One() noexcept
{
    return T(1);
}

template<cArithmeticType T> BEE_FUNC constexpr T Two() noexcept
{
    return T(2);
}

template<cArithmeticType T> BEE_FUNC constexpr Float Radians(T x) noexcept
{
    return static_cast<Float>(x) * static_cast<Float>(0.01745329251994329576923690768489);
}

template<cArithmeticType T> BEE_FUNC constexpr Float Degrees(T x) noexcept
{
    return static_cast<Float>(x) * static_cast<Float>(57.295779513082320876798154814105);
}

template<cArithmeticType T, i32 N> BEE_FUNC constexpr T Pow(T v) noexcept
{
    if constexpr (N == 0)
        return 1;
    else if constexpr (N == 1)
        return v;
    else if constexpr (N < 0)
        return 1 / Pow<T, -N>(v);
    else {
        auto v_2 = Pow<T, N / 2>(v);
        return v_2 * v_2 * Pow<T, N & 1>(v);
    }
}

template<cArithmeticType T>
BEE_FUNC constexpr T Pow2(T v) noexcept
{
    return Pow<T, 2>(v);
}
} // namespace bee