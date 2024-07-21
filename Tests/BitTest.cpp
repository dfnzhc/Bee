/**
 * @File BitTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/6/10
 * @Brief 
 */

#include <gtest/gtest.h>

#include "Bee/Bee.hpp"

using namespace bee;

TEST(aSqrtTest, SquareRootOfOne)
{
    f32 input    = 1.0f;
    f32 expected = 1.0f;
    f32 result   = aSqrt(input);
    EXPECT_NEAR(expected, result, 1e-5); // Allow a small error margin
}

TEST(aSqrtTest, SquareRootOfTwo)
{
    f32 input    = 2.0f;
    f32 expected = std::sqrt(input);
    f32 result   = aSqrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(aSqrtTest, SquareRootOfPointFive)
{
    f32 input    = 0.5f;
    f32 expected = std::sqrt(input);
    f32 result   = aSqrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(aSqrtTest, EdgeCaseZero)
{
    f32 input    = 0.0f;
    f32 expected = 0.0f;
    f32 result   = aSqrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(aCbrtTest, CubeRootOfOne)
{
    f32 input    = 1.0f;
    f32 expected = 1.0f;
    f32 result   = aCbrt(input);
    EXPECT_NEAR(expected, result, 1e-5); // Allow a small error margin
}

TEST(aCbrtTest, CubeRootOfEight)
{
    f32 input    = 8.0f;
    f32 expected = 2.0f; // Since 2^3 = 8
    f32 result   = aCbrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(aCbrtTest, CubeRootOfPointFive)
{
    f32 input    = 0.5f;
    f32 expected = std::cbrt(input); // Use std::cbrt for expected value
    f32 result   = aCbrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(aCbrtTest, EdgeCaseZero)
{
    f32 input    = 0.0f;
    f32 expected = 0.0f;
    f32 result   = aCbrt(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(rSqrtTest, ReciprocalSquareRootOfOne)
{
    f32 input    = 1.0f;
    f32 expected = 1.0f;                 // The reciprocal square root of 1 is 1.
    f32 result   = rSqrtFast(input);
    EXPECT_NEAR(expected, result, 1e-5); // Allow a small error margin
}

TEST(rSqrtTest, ReciprocalSquareRootOfTwo)
{
    f32 input    = 2.0f;
    f32 expected = 0.70710678f; // The reciprocal square root of 2 is 1/sqrt(2).
    f32 result   = rSqrtFast(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(rSqrtTest, ReciprocalSquareRootOfPointFive)
{
    f32 input    = 0.5f;
    f32 expected = 1.41421354f; // The reciprocal square root of 0.5 is sqrt(2).
    f32 result   = rSqrtFast(input);
    EXPECT_NEAR(expected, result, 1e-5);
}

TEST(rSqrtFastTest, ReciprocalSquareRootOfOne)
{
    f32 input    = 1.0f;
    f32 expected = 1.0f;                 // The reciprocal square root of 1 is 1
    f32 result   = rSqrtFast(input);
    EXPECT_NEAR(expected, result, 5e-2); // Allow a larger error margin due to lower precision
}

TEST(rSqrtFastTest, ReciprocalSquareRootOfTwo)
{
    f32 input = 2.0f;
    // The exact reciprocal square root of 2 is 1/sqrt(2), but we expect some deviation
    f32 result = rSqrtFast(input);
    // Use a less strict tolerance due to the function's lower precision
    EXPECT_NEAR(1.0f / std::sqrt(2.0f), result, 1e-2);
}

TEST(rSqrtFastTest, ReciprocalSquareRootOfPointFive)
{
    f32 input = 0.5f;
    // The exact reciprocal square root of 0.5 is sqrt(2), but we expect some deviation
    f32 result = rSqrtFast(input);
    EXPECT_NEAR(std::sqrt(2.0f), result, 5e-2);
}

TEST(rSqrtdTest, ReciprocalSquareRootOfOne)
{
    f64 input    = 1.0;
    f64 expected = 1.0;                  // The reciprocal square root of 1 is 1
    f64 result   = rSqrtFast(input);
    EXPECT_NEAR(expected, result, 5e-2); // Allow a small error margin
}

TEST(rSqrtdTest, ReciprocalSquareRootOfTwo)
{
    f64 input    = 2.0;
    f64 expected = 1.0 / std::sqrt(2.0); // The reciprocal square root of 2.
    f64 result   = rSqrtFast(input);
    EXPECT_NEAR(expected, result, 5e-2);
}

TEST(BitWidthTest, BitWidthOfUnsigned)
{
    EXPECT_EQ(8, BitWidth<u8>());
    EXPECT_EQ(16, BitWidth<u16>());
    EXPECT_EQ(32, BitWidth<u32>());
    EXPECT_EQ(64, BitWidth<u64>());
}

TEST(RotLeftTest, RotateU8)
{
    uint8_t value    = 0b1110'1010;
    int bits         = 2;
    uint8_t expected = 0b1010'1011;
    EXPECT_EQ(expected, RotLeft(value, bits));
}

TEST(RotLeftTest, RotateU16)
{
    uint16_t value    = 0b1100'1100'1100'1100;
    int bits          = 2;
    uint16_t expected = 0b0011'0011'0011'0011;
    EXPECT_EQ(expected, RotLeft(value, bits));
}

TEST(RotLeftTest, RotateU32)
{
    uint32_t value    = 0b1111'0000'1111'0000'1111'0000'1111'0000;
    int bits          = 4;
    uint32_t expected = 0b0000'1111'0000'1111'0000'1111'0000'1111;
    EXPECT_EQ(expected, RotLeft(value, bits));
}

TEST(RotLeftTest, RotateU64)
{
    uint64_t value    = 0xFF0FF1FFFF2FFF3F;
    int bits          = 16;
    uint64_t expected = 0xF1FFFF2FFF3FFF0F;
    EXPECT_EQ(expected, RotLeft(value, bits));
}

TEST(RotRightTest, RotateU8)
{
    uint8_t value    = 0b10101010;
    int bits         = 2;
    uint8_t expected = 0b10101010;
    EXPECT_EQ(expected, RotRight(value, bits));
}

TEST(RotRightTest, RotateU16)
{
    uint16_t value    = 0xF1F3;
    int bits          = 4;
    uint16_t expected = 0x3F1F;
    EXPECT_EQ(expected, RotRight(value, bits));
}

TEST(RotRightTest, RotateU32)
{
    uint32_t value    = 0xF0F1F2F3;
    int bits          = 8;
    uint32_t expected = 0xF3F0F1F2;
    EXPECT_EQ(expected, RotRight(value, bits));
}

TEST(RotRightTest, RotateU64)
{
    uint64_t value    = 0xFF0FF1FFFF2FFF3F;
    int bits          = 16;
    uint64_t expected = 0xFF3FFF0FF1FFFF2F;
    EXPECT_EQ(expected, RotRight(value, bits));
}

TEST(NumLeadingZeroTest, ZeroValue)
{
    uint32_t value = 0;
    EXPECT_EQ(32u, NumLeadingZero(value)); // Expect all 32 bits to be zero
}

TEST(NumLeadingZeroTest, MidValue)
{
    uint32_t value = 0x00080000;           // Only the highest bit is set
    EXPECT_EQ(12u, NumLeadingZero(value)); // Expect one leading zero (in the 32-bit context)
}

TEST(NumLeadingZeroTest, AllOnes)
{
    uint32_t value = ~0u;                 // All bits are set
    EXPECT_EQ(0u, NumLeadingZero(value)); // No leading zeros
}

TEST(NumLeadingZeroTest, ZeroValueU64)
{
    uint64_t value = 0;
    EXPECT_EQ(64u, NumLeadingZero(value)); // Expect all 64 bits to be zero
}

TEST(NumLeadingZeroTest, MidValueU64)
{
    uint64_t value = 0x0000800000000000;   // Only the highest bit is set
    EXPECT_EQ(16u, NumLeadingZero(value)); // Expect one leading zero
}

TEST(NumLeadingZeroTest, AllOnesU64)
{
    uint64_t value = ~0ull;               // All bits are set
    EXPECT_EQ(0u, NumLeadingZero(value)); // No leading zeros
}

TEST(BitCountTest, ZeroBitsSet)
{
    uint32_t value = 0;
    EXPECT_EQ(0u, BitCount(value)); // Expect no bits set
}

TEST(BitCountTest, OneBitSet)
{
    uint32_t value = 1; // Only the least significant bit is set
    EXPECT_EQ(1u, BitCount(value));
}

TEST(BitCountTest, AllBitsSet)
{
    uint32_t value = ~0u;            // All bits are set
    EXPECT_EQ(32u, BitCount(value)); // Expect all 32 bits set
}

TEST(BitCountTest, OddPatternBitsSet)
{
    uint32_t value = 0b10101010101010101010101010101010; // Odd pattern
    // The expected value is the count of bits set to 1 in the pattern
    uint32_t expected = 16; // Replace with the actual expected the bit count
    EXPECT_EQ(expected, BitCount(value));
}

TEST(ReverseBitsTest, KnownValue)
{
    uint32_t value    = 0b10101010101010101010101010101110; // Known bit pattern
    uint32_t expected = 0b01110101010101010101010101010101; // Expected reversed pattern
    EXPECT_EQ(expected, ReverseBits(value));
}

TEST(ReverseBitsTest, Zero)
{
    uint32_t value = 0;
    EXPECT_EQ(0u, ReverseBits(value)); // Expect zero after reversal
}

TEST(ReverseBitsTest, AllOnes)
{
    uint32_t value    = ~0u;        // All bits set
    uint32_t expected = 0xFFFFFFFF; // Expected all bits set after reversal
    EXPECT_EQ(expected, ReverseBits(value));
}

TEST(ReverseBitsTest, KnownValueU64)
{
    uint64_t value    = 0x000077237A4E9F00;
    uint64_t expected = 0x00F9725EC4EE0000; // Expected reversed pattern
    EXPECT_EQ(expected, ReverseBits(value));
}

TEST(ReverseBitsTest, ZeroU64)
{
    uint64_t value = 0;
    EXPECT_EQ(0u, ReverseBits(value)); // Expect zero after reversal
}

TEST(ReverseBitsTest, AllOnesU64)
{
    uint64_t value    = ~0ull;              // All bits set
    uint64_t expected = 0xFFFFFFFFFFFFFFFF; // Expected all bits set after reversal
    EXPECT_EQ(expected, ReverseBits(value));
}

TEST(Log2IntTest, PositiveValue)
{
    EXPECT_EQ(4, Log2Int(16.0f)); // 2^4 = 16
    EXPECT_EQ(5, Log2Int(32.0));  // 2^5 = 32
}

TEST(Log2IntTest, FractionValue)
{
    EXPECT_EQ(-2, Log2Int(0.25f)); // 1/2^2 = 0.25
    EXPECT_EQ(-1, Log2Int(0.5));   // 1/2 = 0.5
}

TEST(Log2IntTest, EdgeCase_One)
{
    EXPECT_EQ(0, Log2Int(1.0)); // 2^0 = 1
}

TEST(Log2IntTest, PowerOfTwo)
{
    uint32_t value = 16; // 2^4
    EXPECT_EQ(4, Log2Int(value));
}

TEST(Log2IntTest, Zero)
{
    uint32_t value = 0;
    EXPECT_EQ(0, Log2Int(value)); // Log2(0) is undefined, function returns 0
}

TEST(Log2IntTest, AllOnes)
{
    uint32_t value = ~0u; // All bits set, equivalent to 2^32
    EXPECT_EQ(31, Log2Int(value));
}

TEST(Log4IntTest, PowerOfFour)
{
    EXPECT_EQ(2, Log4Int(16)); // 4^2 = 16
    EXPECT_EQ(3, Log4Int(64)); // 4^3 = 64
}

TEST(Log4IntTest, FractionalPowerOfFour)
{
    EXPECT_EQ(-1, Log4Int(0.25)); // 4^-1 = 0.25
}

TEST(IsPowerOf2Test, PositivePowersOf2)
{
    EXPECT_TRUE(IsPowerOf2(1));     // 2^0
    EXPECT_TRUE(IsPowerOf2(2));     // 2^1
    EXPECT_TRUE(IsPowerOf2(1'024)); // 2^10
}

TEST(IsPowerOf2Test, NonPowersOf2)
{
    EXPECT_FALSE(IsPowerOf2(3));     // 3 is not a power of 2
    EXPECT_FALSE(IsPowerOf2(5));     // 5 is not a power of 2
    EXPECT_FALSE(IsPowerOf2(1'023)); // 1023 is not a power of 2
}

TEST(IsPowerOf2Test, Zero)
{
    EXPECT_FALSE(IsPowerOf2(0)); // 0 is not a power of 2
}

TEST(IsPowerOf2Test, NegativeNumbers)
{
    EXPECT_FALSE(IsPowerOf2(-1)); // Negative numbers cannot be powers of 2
}

TEST(IsPowerOf4Test, PositivePowersOf4)
{
    EXPECT_TRUE(IsPowerOf4(1));   // 4^0
    EXPECT_TRUE(IsPowerOf4(4));   // 4^1
    EXPECT_TRUE(IsPowerOf4(256)); // 4^4
}

TEST(IsPowerOf4Test, NonPowersOf4)
{
    EXPECT_FALSE(IsPowerOf4(2));   // 2 is not a power of 4
    EXPECT_FALSE(IsPowerOf4(8));   // 8 is not a power of 4 (2^3)
    EXPECT_FALSE(IsPowerOf4(625)); // 625 is not a power of 4 (5^4)
}

TEST(IsPowerOf4Test, Zero)
{
    EXPECT_FALSE(IsPowerOf4(0)); // 0 is not a power of 4
}

TEST(IsPowerOf4Test, NegativeNumbers)
{
    EXPECT_FALSE(IsPowerOf4(-1)); // Negative numbers cannot be powers of 4
}

TEST(RoundUpPow2Test, AlreadyPowerOf2)
{
    EXPECT_EQ(8, RoundUpPow2(8)); // 8 is already a power of 2
}

TEST(RoundUpPow2Test, NotPowerOf2)
{
    EXPECT_EQ(16, RoundUpPow2(9)); // Closest power of 2 to 9 is 16
}

TEST(RoundUpPow2Test, Zero)
{
    EXPECT_EQ(1, RoundUpPow2(0)); // Smallest power of 2 is 1
}
TEST(RoundDownPow2Test, AlreadyPowerOf2)
{
    EXPECT_EQ(8, RoundDownPow2(8));      // 8 is already a power of 2
    EXPECT_EQ(8, RoundDownPow2(u64(8))); // 8 is already a power of 2
}

TEST(RoundDownPow2Test, AbovePowerOf2)
{
    EXPECT_EQ(8, RoundDownPow2(11));      // Closest power of 2 below 11 is 8
    EXPECT_EQ(8, RoundDownPow2(u64(11))); // Closest power of 2 below 11 is 8
}

TEST(RoundDownPow2Test, Zero)
{
    EXPECT_EQ(0, RoundDownPow2(0));      // The only power of 2 less than or equal to 0 is 0
    EXPECT_EQ(0, RoundDownPow2(u64(0))); // The only power of 2 less than or equal to 0 is 0
}

TEST(RoundUpTest, PositiveValues)
{
    EXPECT_EQ(6, RoundUp(4, 3)); // 4 rounded up to the nearest multiple of 3 is 6
}

TEST(RoundUpTest, AlreadyMultiple)
{
    EXPECT_EQ(6, RoundUp(6, 3)); // 6 is already a multiple of 3
}

TEST(RoundUpTest, Zero)
{
    EXPECT_EQ(3, RoundUp(0, 3)); // 0 rounded up to the nearest multiple of 3 is 3
}

TEST(ParityTest, EvenParity)
{
    uint32_t value = 0b10'1010'1010'1010'1010; // Odd number of set bits
    EXPECT_EQ(1u, Parity(value));              // Expect even parity
}

TEST(ParityTest, OddParity)
{
    uint32_t value = 0b1111'0000'1111'0000; // Even number of set bits
    EXPECT_EQ(0u, Parity(value));           // Expect odd parity
}

TEST(ParityTest, AllZeros)
{
    uint32_t value = 0;           // All bits are zero
    EXPECT_EQ(0u, Parity(value)); // Expect even parity (no bits set)
}

TEST(ParityTest, AllOnes)
{
    uint32_t value = ~0u;         // All bits are set
    EXPECT_EQ(0u, Parity(value)); // Expect odd parity (all bits set)
}

TEST(ShuffleTest, KnownPattern)
{
    uint32_t value        = 0x12345678; // Known bit pattern
    const auto shuffled   = Shuffle(value);
    const auto unShuffled = Unshuffle(shuffled);

    EXPECT_EQ(value, unShuffled);
}
