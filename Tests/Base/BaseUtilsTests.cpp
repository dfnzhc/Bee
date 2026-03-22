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
#include "Base/Threading.hpp"

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

TEST(BaseThreadingTests, ThreadPauseIsCallable)
{
    for (std::uint32_t i = 0; i < 256; ++i) {
        bee::ThreadPause(i);
    }
    SUCCEED();
}

TEST(BaseThreadingTests, ThreadPauseVariantsAndContentionHelpersAreCallable)
{
    bee::ThreadYield();
    bee::ThreadPauseRelaxed();
    bee::ThreadPauseWithYield(63, 0x3Fu);

    std::uint32_t spin_count = 0;
    bee::OnTryContention(spin_count, 0x01u);
    EXPECT_EQ(spin_count, 1u);

    bee::AdaptiveSpinWait(spin_count, 0x03u, 8u, 16u);
    EXPECT_EQ(spin_count, 2u);
}

TEST(BaseThreadingTests, HardwareThreadCountAndThreadIdHashAreAvailable)
{
    EXPECT_GE(bee::HardwareThreadCount(), 1u);
    const auto id_hash = bee::ThreadIdHash();
    EXPECT_EQ(id_hash, bee::ThreadIdHash());
}

TEST(BaseThreadingTests, SleepUtilitiesAreCallable)
{
    const auto begin = std::chrono::steady_clock::now();
    bee::SleepForNanos(1000);
    bee::SleepForMicros(1);
    const auto end = std::chrono::steady_clock::now();
    EXPECT_GE(end, begin);
}

TEST(BaseThreadingTests, SetCurrentThreadNameAcceptsNonEmptyName)
{
    EXPECT_FALSE(bee::SetCurrentThreadName(std::string_view{}));
    SUCCEED() << bee::SetCurrentThreadName("BaseThreadingTests");
}
