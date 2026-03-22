/**
 * @File BaseUtilsTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/18
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include "Base/Ascii.hpp"
#include "Base/Naming.hpp"
#include "Base/Text.hpp"

TEST(BaseTextTests, TrimSpacesRemovesLeadingAndTrailingSpaces)
{
    EXPECT_EQ(bee::TrimSpaces("  Alpha  "), "Alpha");
}

TEST(BaseTextTests, SliceExtractsTokenBetweenMarkers)
{
    EXPECT_EQ(bee::Slice("prefix[value]suffix", "[", "]"), "value");
}

TEST(BaseAsciiTests, ClassifiesAsciiCharacters)
{
    EXPECT_TRUE(bee::IsAsciiUpper('A'));
    EXPECT_TRUE(bee::IsAsciiLower('z'));
    EXPECT_TRUE(bee::IsAsciiDigit('8'));
    EXPECT_EQ(bee::ToAsciiLower('Q'), 'q');
}

TEST(BaseNamingTests, ToSnakeCaseConvertsPascalCase)
{
    EXPECT_EQ(bee::ToSnakeCase("MoveSpeed"), "move_speed");
}
