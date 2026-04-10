/**
 * @File NumericTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/6/7
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "Base/Numeric.hpp"

// ============================================================================
// Compile-time verification
// ============================================================================

static_assert(bee::SaturatingAdd<std::uint8_t>(100, 50) == 150);
static_assert(bee::SaturatingAdd<std::uint8_t>(200, 100) == 255);
static_assert(bee::SaturatingAdd<std::uint32_t>(0, 0) == 0);
static_assert(bee::SaturatingAdd<std::uint64_t>(UINT64_MAX, 1) == UINT64_MAX);

// ============================================================================
// BaseNumericTests
// ============================================================================

TEST(BaseNumericTests, SaturatingAddNoOverflowUint8)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint8_t>(100, 50), std::uint8_t{150});
}

TEST(BaseNumericTests, SaturatingAddExactMaxUint8)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint8_t>(200, 55), std::uint8_t{255});
}

TEST(BaseNumericTests, SaturatingAddOverflowSaturatesUint8)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint8_t>(200, 100), std::uint8_t{255});
}

TEST(BaseNumericTests, SaturatingAddBothZero)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint32_t>(0, 0), std::uint32_t{0});
}

TEST(BaseNumericTests, SaturatingAddOneZero)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint32_t>(42, 0), std::uint32_t{42});
    EXPECT_EQ(bee::SaturatingAdd<std::uint32_t>(0, 42), std::uint32_t{42});
}

TEST(BaseNumericTests, SaturatingAddMaxPlusZero)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint32_t>(UINT32_MAX, 0), UINT32_MAX);
}

TEST(BaseNumericTests, SaturatingAddMaxPlusOne)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint32_t>(UINT32_MAX, 1), UINT32_MAX);
}

TEST(BaseNumericTests, SaturatingAddMaxPlusMax)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint32_t>(UINT32_MAX, UINT32_MAX), UINT32_MAX);
}

TEST(BaseNumericTests, SaturatingAddUint16NoOverflow)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint16_t>(1000, 2000), std::uint16_t{3000});
}

TEST(BaseNumericTests, SaturatingAddUint16Overflow)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint16_t>(60000, 10000), std::uint16_t{65535});
}

TEST(BaseNumericTests, SaturatingAddUint64NoOverflow)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint64_t>(1'000'000'000ULL, 2'000'000'000ULL), 3'000'000'000ULL);
}

TEST(BaseNumericTests, SaturatingAddUint64Overflow)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint64_t>(UINT64_MAX, UINT64_MAX), UINT64_MAX);
}

TEST(BaseNumericTests, SaturatingAddUint64MaxPlusOne)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint64_t>(UINT64_MAX, 1), UINT64_MAX);
}
