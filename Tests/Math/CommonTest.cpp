/**
 * @File CommonTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Math.hpp>
#include <libassert/assert-gtest.hpp>

using namespace bee;

TEST(MathCommon, TestMin)
{
    EXPECT_EQ(Min<i32>(42, 27), 27);
    EXPECT_EQ(Min<i32>(42, 42, 27), 27);
    EXPECT_EQ(Min<i32>(42, 42, 27, 18), 18);
}

TEST(MathCommon, TestMax)
{
    EXPECT_EQ(Max<u32>(42, 27), 42);
    EXPECT_EQ(Max<u32>(42, 42, 27), 42);
    EXPECT_EQ(Max<u32>(42, 42, 27, 18), 42);
}

TEST(MathCommon, TestEqual)
{
    EXPECT_TRUE(Equal<i32>(42, 42));
    EXPECT_FALSE(Equal<i32>(42, -42));
}

TEST(MathCommon, TestNotEqual)
{
    EXPECT_TRUE(NotEqual<f32>(42, -42));
    EXPECT_FALSE(NotEqual<f64>(42, 42));
}

TEST(MathCommon, TestApprox)
{
    EXPECT_TRUE(Approx(42., 42.0000001));
    EXPECT_FALSE(Approx(42., 42.5));
}

TEST(MathCommon, TestNotApprox)
{
    EXPECT_TRUE(NotApprox(42., 42.5));
    EXPECT_FALSE(NotApprox(42., 42.0000001));
}

TEST(MathCommon, SquareRootOfOne)
{
    f32 input    = 1.0f;
    f32 expected = 1.0f;
    f32 result   = ApproxSqrt(input);
    EXPECT_NEAR(expected, result, 1e-5); // Allow a small error margin
}

TEST(MathCommon, SquareRootOfTwo)
{
    f32 input    = 2.0f;
    f32 expected = std::sqrt(input);
    f32 result   = ApproxSqrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(MathCommon, SquareRootOfPointFive)
{
    f32 input    = 0.5f;
    f32 expected = std::sqrt(input);
    f32 result   = ApproxSqrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(MathCommon, SquareEdgeCaseZero)
{
    f32 input    = 0.0f;
    f32 expected = 0.0f;
    f32 result   = ApproxSqrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(MathCommon, CubeRootOfOne)
{
    f32 input    = 1.0f;
    f32 expected = 1.0f;
    f32 result   = ApproxCbrt(input);
    EXPECT_NEAR(expected, result, 1e-5); // Allow a small error margin
}

TEST(MathCommon, CubeRootOfEight)
{
    f32 input    = 8.0f;
    f32 expected = 2.0f; // Since 2^3 = 8
    f32 result   = ApproxCbrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(MathCommon, CubeRootOfPointFive)
{
    f32 input    = 0.5f;
    f32 expected = std::cbrt(input); // Use std::cbrt for expected value
    f32 result   = ApproxCbrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(MathCommon, CubeEdgeCaseZero)
{
    f32 input    = 0.0f;
    f32 expected = 0.0f;
    f32 result   = ApproxCbrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(MathCommon, RoundUpPositiveValues)
{
    EXPECT_EQ(6, RoundUp(4, 3)); // 4 rounded up to the nearest multiple of 3 is 6
}

TEST(MathCommon, RoundUpAlreadyMultiple)
{
    EXPECT_EQ(6, RoundUp(6, 3)); // 6 is already a multiple of 3
}

TEST(MathCommon, RoundUpZero)
{
    EXPECT_EQ(3, RoundUp(0, 3)); // 0 rounded up to the nearest multiple of 3 is 3
}

TEST(MathCommon, EvenParity)
{
    uint32_t value = 0b10'1010'1010'1010'1010; // Odd number of set bits
    EXPECT_EQ(1u, Parity(value));              // Expect even parity
}

TEST(MathCommon, OddParity)
{
    uint32_t value = 0b1111'0000'1111'0000; // Even number of set bits
    EXPECT_EQ(0u, Parity(value));           // Expect odd parity
}

TEST(MathCommon, AllZerosParity)
{
    uint32_t value = 0;           // All bits are zero
    EXPECT_EQ(0u, Parity(value)); // Expect even parity (no bits set)
}

TEST(MathCommon, AllOnesParity)
{
    uint32_t value = ~0u;         // All bits are set
    EXPECT_EQ(0u, Parity(value)); // Expect odd parity (all bits set)
}