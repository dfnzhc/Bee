/**
 * @File BaseUtilsTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/18
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string_view>

#include "Base/Ascii.hpp"
#include "Base/Bit.hpp"
#include "Base/Naming.hpp"
#include "Base/Text.hpp"

// ============================================================================
// BaseTextTests
// ============================================================================

TEST(BaseTextTests, TrimSpacesRemovesLeadingAndTrailingSpaces)
{
    EXPECT_EQ(bee::TrimSpaces("  Alpha  "), "Alpha");
}

TEST(BaseTextTests, SliceExtractsTokenBetweenMarkers)
{
    EXPECT_EQ(bee::Slice("prefix[value]suffix", "[", "]"), "value");
}

TEST(BaseTextTests, TrimSpacesEmptyString)
{
    EXPECT_EQ(bee::TrimSpaces(""), "");
}

TEST(BaseTextTests, TrimSpacesOnlySpaces)
{
    EXPECT_EQ(bee::TrimSpaces("     "), "");
}

TEST(BaseTextTests, TrimSpacesSingleNonSpaceChar)
{
    EXPECT_EQ(bee::TrimSpaces("X"), "X");
}

TEST(BaseTextTests, TrimSpacesNoSpaces)
{
    EXPECT_EQ(bee::TrimSpaces("Hello"), "Hello");
}

TEST(BaseTextTests, TrimSpacesLeadingOnly)
{
    EXPECT_EQ(bee::TrimSpaces("   Hello"), "Hello");
}

TEST(BaseTextTests, TrimSpacesTrailingOnly)
{
    EXPECT_EQ(bee::TrimSpaces("Hello   "), "Hello");
}

TEST(BaseTextTests, TrimSpacesTabsNotTrimmed)
{
    EXPECT_EQ(bee::TrimSpaces("\tHello\t"), "\tHello\t");
}

TEST(BaseTextTests, TrimSpacesSingleSpace)
{
    EXPECT_EQ(bee::TrimSpaces(" "), "");
}

TEST(BaseTextTests, SlicePrefixNotFound)
{
    EXPECT_EQ(bee::Slice("hello world", "[", "]"), "");
}

TEST(BaseTextTests, SlicePrefixNotFoundCustomFallback)
{
    EXPECT_EQ(bee::Slice("hello world", "[", "]", "N/A"), "N/A");
}

TEST(BaseTextTests, SliceSuffixNotFound)
{
    EXPECT_EQ(bee::Slice("hello[world", "[", "]"), "");
}

TEST(BaseTextTests, SliceEmptyPrefixAndSuffix)
{
    EXPECT_EQ(bee::Slice("hello", "", ""), "");
}

TEST(BaseTextTests, SliceAdjacentMarkersReturnsFallback)
{
    EXPECT_EQ(bee::Slice("hello[]world", "[", "]"), "");
}

TEST(BaseTextTests, SliceNestedMarkersFindsFirst)
{
    EXPECT_EQ(bee::Slice("a[b[c]d", "[", "]"), "b[c");
}

TEST(BaseTextTests, SliceEmptyInput)
{
    EXPECT_EQ(bee::Slice("", "[", "]"), "");
}

// ============================================================================
// BaseAsciiTests
// ============================================================================

TEST(BaseAsciiTests, ClassifiesAsciiCharacters)
{
    EXPECT_TRUE(bee::IsAsciiUpper('A'));
    EXPECT_TRUE(bee::IsAsciiLower('z'));
    EXPECT_TRUE(bee::IsAsciiDigit('8'));
    EXPECT_EQ(bee::ToAsciiLower('Q'), 'q');
}

TEST(BaseAsciiTests, BoundaryCharsUpperRange)
{
    EXPECT_FALSE(bee::IsAsciiUpper('@')); // 'A' - 1
    EXPECT_TRUE(bee::IsAsciiUpper('A'));
    EXPECT_TRUE(bee::IsAsciiUpper('Z'));
    EXPECT_FALSE(bee::IsAsciiUpper('[')); // 'Z' + 1
}

TEST(BaseAsciiTests, BoundaryCharsLowerRange)
{
    EXPECT_FALSE(bee::IsAsciiLower('`')); // 'a' - 1
    EXPECT_TRUE(bee::IsAsciiLower('a'));
    EXPECT_TRUE(bee::IsAsciiLower('z'));
    EXPECT_FALSE(bee::IsAsciiLower('{')); // 'z' + 1
}

TEST(BaseAsciiTests, BoundaryCharsDigitRange)
{
    EXPECT_FALSE(bee::IsAsciiDigit('/')); // '0' - 1
    EXPECT_TRUE(bee::IsAsciiDigit('0'));
    EXPECT_TRUE(bee::IsAsciiDigit('9'));
    EXPECT_FALSE(bee::IsAsciiDigit(':')); // '9' + 1
}

TEST(BaseAsciiTests, NullCharClassification)
{
    EXPECT_FALSE(bee::IsAsciiUpper('\0'));
    EXPECT_FALSE(bee::IsAsciiLower('\0'));
    EXPECT_FALSE(bee::IsAsciiDigit('\0'));
    EXPECT_FALSE(bee::IsAsciiAlpha('\0'));
}

TEST(BaseAsciiTests, MaxAsciiCharClassification)
{
    EXPECT_FALSE(bee::IsAsciiUpper(char(127)));
    EXPECT_FALSE(bee::IsAsciiLower(char(127)));
    EXPECT_FALSE(bee::IsAsciiDigit(char(127)));
    EXPECT_FALSE(bee::IsAsciiAlpha(char(127)));
}

TEST(BaseAsciiTests, IsAsciiAlphaLettersReturnTrue)
{
    for (char c = 'A'; c <= 'Z'; ++c)
        EXPECT_TRUE(bee::IsAsciiAlpha(c));
    for (char c = 'a'; c <= 'z'; ++c)
        EXPECT_TRUE(bee::IsAsciiAlpha(c));
}

TEST(BaseAsciiTests, IsAsciiAlphaDigitsReturnFalse)
{
    for (char c = '0'; c <= '9'; ++c)
        EXPECT_FALSE(bee::IsAsciiAlpha(c));
}

TEST(BaseAsciiTests, IsAsciiAlphaSpecialReturnFalse)
{
    EXPECT_FALSE(bee::IsAsciiAlpha(' '));
    EXPECT_FALSE(bee::IsAsciiAlpha('!'));
    EXPECT_FALSE(bee::IsAsciiAlpha('_'));
    EXPECT_FALSE(bee::IsAsciiAlpha('-'));
}

TEST(BaseAsciiTests, ToAsciiLowerAlreadyLowercaseUnchanged)
{
    for (char c = 'a'; c <= 'z'; ++c)
        EXPECT_EQ(bee::ToAsciiLower(c), c);
}

TEST(BaseAsciiTests, ToAsciiLowerDigitsUnchanged)
{
    for (char c = '0'; c <= '9'; ++c)
        EXPECT_EQ(bee::ToAsciiLower(c), c);
}

TEST(BaseAsciiTests, ToAsciiLowerSpecialCharsUnchanged)
{
    EXPECT_EQ(bee::ToAsciiLower('!'), '!');
    EXPECT_EQ(bee::ToAsciiLower(' '), ' ');
    EXPECT_EQ(bee::ToAsciiLower('\0'), '\0');
    EXPECT_EQ(bee::ToAsciiLower('_'), '_');
}

TEST(BaseAsciiTests, ToAsciiLowerAllUppercaseConversions)
{
    for (char c = 'A'; c <= 'Z'; ++c)
        EXPECT_EQ(bee::ToAsciiLower(c), static_cast<char>(c - 'A' + 'a'));
}

// ============================================================================
// BaseNamingTests
// ============================================================================

TEST(BaseNamingTests, ToSnakeCaseConvertsPascalCase)
{
    EXPECT_EQ(bee::ToSnakeCase("MoveSpeed"), "move_speed");
}

TEST(BaseNamingTests, ToSnakeCaseEmptyString)
{
    EXPECT_EQ(bee::ToSnakeCase(""), "");
}

TEST(BaseNamingTests, ToSnakeCaseAlreadySnake)
{
    EXPECT_EQ(bee::ToSnakeCase("already_snake"), "already_snake");
}

TEST(BaseNamingTests, ToSnakeCaseCamelCase)
{
    EXPECT_EQ(bee::ToSnakeCase("moveSpeed"), "move_speed");
}

TEST(BaseNamingTests, ToSnakeCaseConsecutiveCapitals)
{
    EXPECT_EQ(bee::ToSnakeCase("XMLParser"), "xml_parser");
}

TEST(BaseNamingTests, ToSnakeCaseConsecutiveCapitalsInMiddle)
{
    EXPECT_EQ(bee::ToSnakeCase("getHTTPSResponse"), "get_https_response");
}

TEST(BaseNamingTests, ToSnakeCaseAllUppercase)
{
    EXPECT_EQ(bee::ToSnakeCase("ABC"), "abc");
}

TEST(BaseNamingTests, ToSnakeCaseSingleUpperChar)
{
    EXPECT_EQ(bee::ToSnakeCase("A"), "a");
}

TEST(BaseNamingTests, ToSnakeCaseSingleLowerChar)
{
    EXPECT_EQ(bee::ToSnakeCase("a"), "a");
}

TEST(BaseNamingTests, ToSnakeCaseDigitsAtEnd)
{
    EXPECT_EQ(bee::ToSnakeCase("Vec3D"), "vec3_d");
}

TEST(BaseNamingTests, ToSnakeCaseLeadingUnderscores)
{
    EXPECT_EQ(bee::ToSnakeCase("__test"), "test");
}

TEST(BaseNamingTests, ToSnakeCaseTrailingUnderscores)
{
    EXPECT_EQ(bee::ToSnakeCase("test__"), "test");
}

TEST(BaseNamingTests, ToSnakeCaseMixedDelimiters)
{
    EXPECT_EQ(bee::ToSnakeCase("my-var"), "my_var");
}

// ============================================================================
// BaseBitOpsTests
// ============================================================================

TEST(BaseBitOpsTests, PowerOfTwoUtilitiesWork)
{
    EXPECT_TRUE(bee::IsPowerOfTwo(1u));
    EXPECT_TRUE(bee::IsPowerOfTwo(8u));
    EXPECT_FALSE(bee::IsPowerOfTwo(0u));
    EXPECT_FALSE(bee::IsPowerOfTwo(10u));
    EXPECT_EQ(bee::RoundUpPowerOfTwo(9u), 16u);
    EXPECT_EQ(bee::HighestPowerOfTwoLEQ(19u), 16u);
}

TEST(BaseBitOpsTests, SupportsDifferentUnsignedTypes)
{
    EXPECT_EQ(bee::RoundUpPowerOfTwo<std::uint32_t>(257u), 512u);
    EXPECT_EQ(bee::HighestPowerOfTwoLEQ<std::uint32_t>(257u), 256u);
}

TEST(BaseBitOpsTests, ReturnsZeroWhenBitCeilWouldOverflow)
{
    EXPECT_EQ(bee::RoundUpPowerOfTwo<std::uint8_t>(129u), static_cast<std::uint8_t>(0));
}

TEST(BaseBitOpsTests, ByteSwapWorksForUnsignedAndSigned)
{
    EXPECT_EQ(bee::ByteSwap<std::uint16_t>(0x00FFu), 0xFF00u);
    EXPECT_EQ(bee::ByteSwap<std::uint32_t>(0x12345678u), 0x78563412u);
    EXPECT_EQ(bee::ByteSwap<std::int32_t>(0x01020304), 0x04030201);
}

TEST(BaseBitOpsTests, EndianConversionRoundTrip)
{
    constexpr std::uint32_t raw = 0x11223344u;
    const auto be               = bee::ToBigEndian(raw);
    const auto le               = bee::ToLittleEndian(raw);
    EXPECT_EQ(bee::FromBigEndian(be), raw);
    EXPECT_EQ(bee::FromLittleEndian(le), raw);
}

TEST(BaseBitOpsTests, IsPowerOfTwoAllPowersUint8)
{
    for (unsigned i = 0; i < 8; ++i)
        EXPECT_TRUE(bee::IsPowerOfTwo(static_cast<std::uint8_t>(1u << i)));
}

TEST(BaseBitOpsTests, IsPowerOfTwoAllPowersUint16)
{
    for (unsigned i = 0; i < 16; ++i)
        EXPECT_TRUE(bee::IsPowerOfTwo(static_cast<std::uint16_t>(1u << i)));
}

TEST(BaseBitOpsTests, IsPowerOfTwoAllPowersUint32)
{
    for (unsigned i = 0; i < 32; ++i)
        EXPECT_TRUE(bee::IsPowerOfTwo(std::uint32_t{1} << i));
}

TEST(BaseBitOpsTests, IsPowerOfTwoAllPowersUint64)
{
    for (unsigned i = 0; i < 64; ++i)
        EXPECT_TRUE(bee::IsPowerOfTwo(std::uint64_t{1} << i));
}

TEST(BaseBitOpsTests, IsPowerOfTwoMaxValues)
{
    EXPECT_FALSE(bee::IsPowerOfTwo(std::numeric_limits<std::uint8_t>::max()));
    EXPECT_FALSE(bee::IsPowerOfTwo(std::numeric_limits<std::uint16_t>::max()));
    EXPECT_FALSE(bee::IsPowerOfTwo(std::numeric_limits<std::uint32_t>::max()));
    EXPECT_FALSE(bee::IsPowerOfTwo(std::numeric_limits<std::uint64_t>::max()));
}

TEST(BaseBitOpsTests, RoundUpPowerOfTwoZeroReturnsOne)
{
    EXPECT_EQ(bee::RoundUpPowerOfTwo(0u), 1u);
}

TEST(BaseBitOpsTests, RoundUpPowerOfTwoOneReturnsOne)
{
    EXPECT_EQ(bee::RoundUpPowerOfTwo(1u), 1u);
}

TEST(BaseBitOpsTests, RoundUpPowerOfTwoExactPowers)
{
    EXPECT_EQ(bee::RoundUpPowerOfTwo(16u), 16u);
    EXPECT_EQ(bee::RoundUpPowerOfTwo(256u), 256u);
    EXPECT_EQ(bee::RoundUpPowerOfTwo(1024u), 1024u);
}

TEST(BaseBitOpsTests, RoundUpPowerOfTwoUint64Large)
{
    constexpr auto val = std::uint64_t{1} << 40;
    EXPECT_EQ(bee::RoundUpPowerOfTwo(val + 1), std::uint64_t{1} << 41);
    EXPECT_EQ(bee::RoundUpPowerOfTwo(val), val);
}

TEST(BaseBitOpsTests, RoundUpPowerOfTwoUint64Overflow)
{
    constexpr auto half = (std::uint64_t{1} << 63) + 1;
    EXPECT_EQ(bee::RoundUpPowerOfTwo(half), std::uint64_t{0});
}

TEST(BaseBitOpsTests, HighestPowerOfTwoLEQZero)
{
    EXPECT_EQ(bee::HighestPowerOfTwoLEQ(0u), 0u);
}

TEST(BaseBitOpsTests, HighestPowerOfTwoLEQOne)
{
    EXPECT_EQ(bee::HighestPowerOfTwoLEQ(1u), 1u);
}

TEST(BaseBitOpsTests, HighestPowerOfTwoLEQExactPowers)
{
    EXPECT_EQ(bee::HighestPowerOfTwoLEQ(32u), 32u);
    EXPECT_EQ(bee::HighestPowerOfTwoLEQ(1024u), 1024u);
}

TEST(BaseBitOpsTests, ByteSwapUint8Identity)
{
    EXPECT_EQ(bee::ByteSwap<std::uint8_t>(0xABu), std::uint8_t{0xABu});
}

TEST(BaseBitOpsTests, ByteSwapUint64)
{
    EXPECT_EQ(bee::ByteSwap<std::uint64_t>(0x0102030405060708ULL), 0x0807060504030201ULL);
}

TEST(BaseBitOpsTests, ByteSwapZero)
{
    EXPECT_EQ(bee::ByteSwap<std::uint32_t>(0u), 0u);
    EXPECT_EQ(bee::ByteSwap<std::uint64_t>(0ULL), 0ULL);
}

TEST(BaseBitOpsTests, ByteSwapNegativeInt16)
{
    constexpr std::int16_t val     = -1; // 0xFFFF
    constexpr std::int16_t swapped = bee::ByteSwap(val);
    EXPECT_EQ(swapped, std::int16_t{-1}); // 0xFFFF swapped is still 0xFFFF
}

TEST(BaseBitOpsTests, EndianMutualExclusivity)
{
    EXPECT_NE(bee::IsBigEndian(), bee::IsLittleEndian());
    EXPECT_TRUE(bee::IsBigEndian() || bee::IsLittleEndian());
}

TEST(BaseBitOpsTests, EndianRoundTripInt16)
{
    constexpr std::int16_t raw = 0x1234;
    EXPECT_EQ(bee::FromBigEndian(bee::ToBigEndian(raw)), raw);
    EXPECT_EQ(bee::FromLittleEndian(bee::ToLittleEndian(raw)), raw);
}

TEST(BaseBitOpsTests, EndianRoundTripInt64)
{
    constexpr std::int64_t raw = 0x0102030405060708LL;
    EXPECT_EQ(bee::FromBigEndian(bee::ToBigEndian(raw)), raw);
    EXPECT_EQ(bee::FromLittleEndian(bee::ToLittleEndian(raw)), raw);
}

TEST(BaseBitOpsTests, NativeEndianIsIdentity)
{
    constexpr std::uint32_t val = 0xDEADBEEFu;
    if constexpr (bee::IsLittleEndian()) {
        EXPECT_EQ(bee::ToLittleEndian(val), val);
        EXPECT_EQ(bee::FromLittleEndian(val), val);
    } else {
        EXPECT_EQ(bee::ToBigEndian(val), val);
        EXPECT_EQ(bee::FromBigEndian(val), val);
    }
}
