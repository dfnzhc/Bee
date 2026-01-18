/**
 * @File CommonTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee/Math/Common.hpp>

using namespace bee;

TEST(Common_SafeArcTrigs, SafeArcSinClampAndRange)
{
    EXPECT_FLOAT_EQ(SafeArcSin(To<f32>(-1)), -PiOver2<f32>());
    EXPECT_FLOAT_EQ(SafeArcSin(To<f32>(0)), 0.f);
    EXPECT_FLOAT_EQ(SafeArcSin(To<f32>(1)), PiOver2<f32>());

    EXPECT_FLOAT_EQ(SafeArcSin(To<f32>(0.5f)), std::asin(To<f32>(0.5f)));

    EXPECT_FLOAT_EQ(SafeArcSin(To<f32>(1.00005f)), PiOver2<f32>());
    EXPECT_FLOAT_EQ(SafeArcSin(To<f32>(-1.00005f)), -PiOver2<f32>());
}

TEST(Common_SafeArcTrigs, SafeArcCosClampAndRange)
{
    EXPECT_FLOAT_EQ(SafeArcCos(To<f32>(1)), 0.f);
    EXPECT_FLOAT_EQ(SafeArcCos(To<f32>(-1)), Pi<f32>());
    EXPECT_FLOAT_EQ(SafeArcCos(To<f32>(0)), PiOver2<f32>());

    EXPECT_FLOAT_EQ(SafeArcCos(To<f32>(0.5f)), std::acos(To<f32>(0.5f)));

    EXPECT_FLOAT_EQ(SafeArcCos(To<f32>(1.00005f)), 0.f);
    EXPECT_FLOAT_EQ(SafeArcCos(To<f32>(-1.00005f)), Pi<f32>());
}

TEST(Common_Core_MinMaxAbs, Basic)
{
    EXPECT_EQ(Abs(-3), 3);
    EXPECT_EQ(Abs(3u), 3u);
    EXPECT_EQ(Min(3, 5), 3);
    EXPECT_EQ(Max(3, 5), 5);
    EXPECT_EQ(Min(5, 3, 7, 2), 2);
    EXPECT_EQ(Max(5, 3, 7, 2), 7);
}

TEST(Common_IntegerMods, Mod)
{
    EXPECT_EQ(Mod(7, 3), 1);
    EXPECT_EQ(Mod(-7, 3), 2);
}

TEST(Common_FloatOpsBasics, NearChecksRoundings)
{
    EXPECT_TRUE(NearZero(1e-7f));
    EXPECT_TRUE(NearOne(1.f));
}

TEST(Common_FloatBits, BitsExponentSign)
{
    auto b = FloatToBits(1.f);
    EXPECT_GT(b, 0u);
    EXPECT_EQ(BitsToFloat(b), 1.f);
    EXPECT_EQ(Exponent(1.f), 0);
    EXPECT_EQ(SignBit(-1.f), 0x80000000u);
}

TEST(Common_NextFloat, NextUpDown)
{
    auto x = 1.f;
    auto up = NextFloatUp(x);
    auto down = NextFloatDown(x);
    EXPECT_GT(up, x);
    EXPECT_LT(down, x);
}

TEST(Common_RoundingDirected, AddMulDivSqrtRound)
{
    EXPECT_GT(AddRoundUp(1.f, 1.f), 2.f - kEpsilonF);
    EXPECT_LT(AddRoundDown(1.f, 1.f), 2.f + kEpsilonF);
    EXPECT_GT(MulRoundUp(2.f, 3.f), 6.f - kEpsilonF);
    EXPECT_LT(MulRoundDown(2.f, 3.f), 6.f + kEpsilonF);
    EXPECT_GT(DivRoundUp(1.f, 3.f), (1.f / 3.f) - kEpsilonF);
    EXPECT_LT(DivRoundDown(1.f, 3.f), (1.f / 3.f) + kEpsilonF);
    EXPECT_GT(SqrtRoundUp(4.f), 2.f - kEpsilonF);
    EXPECT_LT(SqrtRoundDown(4.f), 2.f + kEpsilonF);
}

TEST(Common_GammaClampSaturate, Clamp)
{
    EXPECT_EQ(Saturate(-2.f), 0.f);
    EXPECT_EQ(Saturate(2.f), 1.f);
}

TEST(Common_Steps, SmoothStepSmootherStep)
{
    EXPECT_FLOAT_EQ(SmoothStep(0.f), 0.f);
    EXPECT_FLOAT_EQ(SmoothStep(1.f), 1.f);
    EXPECT_FLOAT_EQ(SmootherStep(0.f), 0.f);
    EXPECT_FLOAT_EQ(SmootherStep(1.f), 1.f);
}

TEST(Common_Others, AlmostZeroSincSqrAngles)
{
    EXPECT_TRUE(AlmostZero(To<f32>(1e-7f)));
    EXPECT_FLOAT_EQ(Sinc(0.1f), std::sin(0.1f) / 0.1f);
    EXPECT_EQ(Sqr(3), 9);
    EXPECT_FLOAT_EQ(Degree2Radian(180.f), Pi<f32>());
    EXPECT_FLOAT_EQ(Radian2Degree(Pi<f32>()), 180.f);
}

TEST(Common_NumberTheory, GCDLCM)
{
    EXPECT_EQ(GCD(12, 18), 6);
    EXPECT_EQ(LCM(12, 18), 36);
}

TEST(Common_MidpointLerp, MidpointAndLerp)
{
    EXPECT_FLOAT_EQ(std::midpoint(0.f, 2.f), 1.f);
    EXPECT_FLOAT_EQ(Lerp(0.f, 2.f, 0.25f), 0.5f);
}

TEST(Common_PolynomialQuadratic, EvaluateAndSolve)
{
    auto v = EvaluatePolynomial(2.f, 1.f, 2.f, 3.f);
    EXPECT_FLOAT_EQ(v, 1.f + 2.f * 2.f + 3.f * 4.f);
    f32 t0{}, t1{};
    EXPECT_TRUE(Quadratic(1.f, -3.f, 2.f, &t0, &t1));
    EXPECT_FLOAT_EQ(t0, 1.f);
    EXPECT_FLOAT_EQ(t1, 2.f);
}

TEST(Common_FastFunctions, FastExpSqrtCbrtInvSqrt)
{
    EXPECT_FLOAT_EQ(FastExp(0.f), 1.f);
    EXPECT_FLOAT_EQ(FastSqrt(4.f), 2.f);
    EXPECT_FLOAT_EQ(FastCbrt(8.f), 2.f);
    EXPECT_NEAR(FastInvSqrt(4.f), 0.5f, 1e-5);
}

TEST(Common_Gaussian, GaussianAndIntegral)
{
    EXPECT_FLOAT_EQ(Gaussian(0.f, 0.f, 1.f), 1.f / std::sqrt(TwoPi<f32>()));
    auto gi = GaussianIntegral(-1.f, 1.f, 0.f, 1.f);
    EXPECT_GT(gi, 0.0);
    EXPECT_LT(gi, 1.0);
}

TEST(Common_Trig, BasicWrappers)
{
    EXPECT_FLOAT_EQ(Sin(0.f), 0.f);
    EXPECT_FLOAT_EQ(Cos(0.f), 1.f);
    EXPECT_FLOAT_EQ(Tan(0.f), 0.f);
    EXPECT_FLOAT_EQ(Asin(0.f), 0.f);
    EXPECT_FLOAT_EQ(Acos(1.f), 0.f);
    EXPECT_FLOAT_EQ(Atan2(0.f, 1.f), 0.f);
}

TEST(Common_Utils, Clamp)
{
    EXPECT_EQ(Clamp(5, 0, 10), 5);
    EXPECT_EQ(Clamp(-5, 0, 10), 0);
    EXPECT_EQ(Clamp(15, 0, 10), 10);
    EXPECT_FLOAT_EQ(Clamp(0.5f, 0.f, 1.f), 0.5f);
    EXPECT_FLOAT_EQ(Clamp(-0.5f, 0.f, 1.f), 0.f);
    EXPECT_FLOAT_EQ(Clamp(1.5f, 0.f, 1.f), 1.f);
}

TEST(Common_Utils, Erf)
{
    EXPECT_FLOAT_EQ(Erf(0.f), 0.f);
    // erf(1) approx 0.8427
    EXPECT_NEAR(Erf(1.f), 0.8427f, 1e-4f);
}

TEST(Common_Math, InvSqrtHypot)
{
    EXPECT_FLOAT_EQ(InvSqrt(4.f), 0.5f);
    EXPECT_FLOAT_EQ(InvHypot(3.f, 4.f), 0.2f); // 1/5
}

TEST(Common_Math, Rounding)
{
    EXPECT_FLOAT_EQ(Floor(1.5f), 1.f);
    EXPECT_FLOAT_EQ(Floor(-1.5f), -2.f);
    EXPECT_FLOAT_EQ(Ceil(1.5f), 2.f);
    EXPECT_FLOAT_EQ(Ceil(-1.5f), -1.f);
}

TEST(Common_Math, FMA_CopySign)
{
    EXPECT_FLOAT_EQ(FMA(2.f, 3.f, 4.f), 10.f);
    EXPECT_FLOAT_EQ(CopySign(1.f, -2.f), -1.f);
    EXPECT_FLOAT_EQ(CopySign(-1.f, 2.f), 1.f);
}

TEST(Common_Checks, FiniteInfNaN)
{
    EXPECT_TRUE(IsFinite(1.f));
    EXPECT_FALSE(IsFinite(kInfinityF));
    EXPECT_FALSE(IsFinite(std::numeric_limits<f32>::quiet_NaN()));

    EXPECT_FALSE(IsInf(1.f));
    EXPECT_TRUE(IsInf(kInfinityF));
    EXPECT_FALSE(IsInf(std::numeric_limits<f32>::quiet_NaN()));

    EXPECT_FALSE(IsNaN(1.f));
    EXPECT_FALSE(IsNaN(kInfinityF));
    EXPECT_TRUE(IsNaN(std::numeric_limits<f32>::quiet_NaN()));
}

TEST(Common_Math, Sqrt)
{
    EXPECT_FLOAT_EQ(Sqrt(4.f), 2.f);
    EXPECT_FLOAT_EQ(Sqrt(9.f), 3.f);
}

TEST(Common_Math, IsNearAbsolute)
{
    EXPECT_TRUE(IsNear(1.0f, 1.0001f, 0.001f));
    EXPECT_TRUE(IsNear(1.0, 1.0001, 0.001));
    EXPECT_FALSE(IsNear(1.0f, 1.01f, 0.001f));

    // Test near zero
    EXPECT_TRUE(IsNear(0.0f, 0.0001f, 0.001f));
}

TEST(Common_Math, IsNearRelative)
{
    EXPECT_TRUE(IsNearRel(1.0f, 1.001f, 0.001f));
    EXPECT_TRUE(IsNearRel(1000.0f, 1001.0f, 0.001f));
    EXPECT_FALSE(IsNearRel(1.0f, 1.01f, 0.001f));

    // Test large values
    EXPECT_TRUE(IsNearRel(1e6f, 1.001e6f, 0.001f));
}
