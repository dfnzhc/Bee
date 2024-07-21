/**
 * @File FloatTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/6/10
 * @Brief 
 */

#include <gtest/gtest.h>

#include "Bee/Bee.hpp"

using namespace bee;

TEST(FmaTest, BasicOperation)
{
    float a        = 1.0f;
    float b        = 2.0f;
    float c        = 3.0f;
    float result   = Fma(a, b, c);
    float expected = a * b + c;          // Expected result without FMA for comparison
    EXPECT_NEAR(expected, result, 1e-6); // Allow a small error margin
}

TEST(FloatToBitsTest, FloatValue)
{
    float f       = 1.0f;
    uint32_t bits = FloatToBits(f);
    // The expected bit pattern for 1.0f as a 32-bit float
    EXPECT_EQ(0x3F800000, bits);
}

TEST(FloatToBitsTest, DoubleValue)
{
    double d      = 1.0;
    uint64_t bits = FloatToBits(d);
    // The expected bit pattern for 1.0 as a 64-bit double
    // This is just an example and may not be the actual bit pattern
    EXPECT_EQ(0x3FF0000000000000, bits);
}

TEST(FloatToBitsTest, NaNValue)
{
    auto nan      = cast_to<float>(std::nan("0"));
    uint32_t bits = FloatToBits(nan);
    // Check if the result is a quiet NaN (all exponent bits set, fraction not zero)
    EXPECT_TRUE((bits & 0x7F800000) == 0x7F800000 && (bits & 0x007FFFFF) != 0);
}

TEST(BitsToFloatTest, FloatBits)
{
    uint32_t bits = 0x3F800000; // Bit pattern for 1.0f as a 32-bit float
    float result  = BitsToFloat(bits);
    EXPECT_EQ(1.0f, result);
}

TEST(BitsToFloatTest, DoubleBits)
{
    uint64_t bits = 0x3FF0000000000000; // Bit pattern for 1.0 as a 64-bit double
    double result = BitsToFloat(bits);
    EXPECT_EQ(1.0, result);
}

TEST(BitsToFloatTest, NaNBits)
{
    uint32_t nanBits = 0x7FC00000; // Bit pattern for a quiet NaN
    float result     = BitsToFloat(nanBits);
    // Check if the result is a NaN (not a number)
    EXPECT_TRUE(std::isnan(result));
}

TEST(BitsToFloatTest, InfinityBits)
{
    uint32_t infBits = 0x7F800000; // Bit pattern for positive infinity in a 32-bit float
    float result     = BitsToFloat(infBits);
    // Check if the result is positive infinity
    EXPECT_TRUE(std::isinf(result) && result > 0);
}

TEST(SignificandTest, FloatValue)
{
    float f    = 3.14159f; // A known float value 0x40490FD0
    u32 result = Significand(f);
    // The expected significand can be calculated or known from the value
    u32 expected = 0x490FD0; // Replace with the actual expected significand for 3.14159f
    EXPECT_EQ(expected, result);
}

TEST(SignificandTest, DoubleValue)
{
    double d   = 3.14159; // A known double value 0x400921F9F01B866E
    u64 result = Significand(d);
    // The expected significand can be calculated or known from the value
    u64 expected = 0x921F9F01B866E; // Replace with the actual expected significand for 3.14159
    EXPECT_EQ(expected, result);
}

TEST(SignBitTest, PositiveNumber)
{
    EXPECT_EQ(0u, SignBit(1.0f)); // Positive number has a sign bit of 0
    EXPECT_EQ(0u, SignBit(1.0));  // Positive number has a sign bit of 0
}

TEST(SignBitTest, NegativeNumber)
{
    EXPECT_EQ(0x80000000u, SignBit(-1.0f));
    EXPECT_EQ(0x8000000000000000u, SignBit(-1.0));
}

TEST(SignBitTest, Zero)
{
    EXPECT_EQ(0u, SignBit(+0.0f));
    EXPECT_EQ(0x80000000u, SignBit(-0.0f));
    EXPECT_EQ(0u, SignBit(+0.0));
    EXPECT_EQ(0x8000000000000000u, SignBit(-0.0));
}

TEST(NextFloatUpTest, PositiveNumber)
{
    float v    = 1.0f;
    float next = NextFloatUp(v);
    // The expected result should be just above 1.0f
    EXPECT_GT(next, v);
    EXPECT_LE(next, v + std::numeric_limits<f32>::epsilon());
}

TEST(NextFloatUpTest, Zero)
{
    float v    = 0.0f;
    float next = NextFloatUp(v);
    // The next representable number after zero is the smallest positive normal value
    EXPECT_GT(next, v);
    EXPECT_LE(next, std::numeric_limits<float>::min());
}

TEST(NextFloatUpTest, NegativeZero)
{
    float v    = -0.0f;
    float next = NextFloatUp(v);
    // The next representable number after negative zero is the smallest positive normal value
    EXPECT_GT(next, v);
    EXPECT_LE(next, std::numeric_limits<float>::min());
}

TEST(NextFloatUpTest, Infinity)
{
    float v    = std::numeric_limits<float>::infinity();
    float next = NextFloatUp(v);
    // Positive infinity is the largest representable number
    EXPECT_EQ(next, v);
}

TEST(NextFloatDownTest, PositiveNumber)
{
    float v   = 1.0f;
    auto next = NextFloatDown(v);
    // The expected result should be just below 1.0f
    EXPECT_LT(next, v);
    EXPECT_GT(next, v - std::numeric_limits<float>::epsilon());
}

TEST(NextFloatDownTest, Zero)
{
    // The next representable number below zero is the nan
    EXPECT_TRUE(std::isnan(NextFloatDown(-0.)));
    EXPECT_TRUE(std::isnan(NextFloatDown(+0.)));
}

TEST(NextFloatDownTest, Infinity)
{
    float v   = -std::numeric_limits<float>::infinity();
    auto next = NextFloatDown(v);
    // Negative infinity is the smallest representable number
    EXPECT_EQ(next, v);
}