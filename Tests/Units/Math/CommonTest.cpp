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
