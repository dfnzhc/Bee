/**
 * @File HashTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/6/7
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <set>

#include "Base/Numeric/Hash.hpp"

// ============================================================================
// Compile-time verification
// ============================================================================

static_assert(bee::Splitmix64(0) == 0);
static_assert(bee::Splitmix64(1) == 0x5692161D100B05E5ULL);
static_assert(bee::Splitmix64(2) == 0xDBD238973A2B148AULL);

// ============================================================================
// BaseHashTests
// ============================================================================

TEST(BaseHashTests, Splitmix64Deterministic)
{
    EXPECT_EQ(bee::Splitmix64(42), bee::Splitmix64(42));
    EXPECT_EQ(bee::Splitmix64(0), bee::Splitmix64(0));
    EXPECT_EQ(bee::Splitmix64(UINT64_MAX), bee::Splitmix64(UINT64_MAX));
}

TEST(BaseHashTests, Splitmix64DifferentInputsDifferentOutputs)
{
    std::set<bee::u64> outputs;
    for (bee::u64 i = 1; i <= 100; ++i)
        outputs.insert(bee::Splitmix64(i));
    EXPECT_EQ(outputs.size(), 100u);
}

TEST(BaseHashTests, Splitmix64ZeroIsFixedPoint)
{
    EXPECT_EQ(bee::Splitmix64(0), bee::u64{0});
}

TEST(BaseHashTests, Splitmix64KnownValues)
{
    EXPECT_EQ(bee::Splitmix64(1), 0x5692161D100B05E5ULL);
    EXPECT_EQ(bee::Splitmix64(2), 0xDBD238973A2B148AULL);
    EXPECT_EQ(bee::Splitmix64(3), 0x1E535EEDE31428F0ULL);
}

TEST(BaseHashTests, Splitmix64MaxInput)
{
    const auto result = bee::Splitmix64(UINT64_MAX);
    EXPECT_EQ(result, 0xB4D055FCF2CBBD7BULL);
}

TEST(BaseHashTests, Splitmix64NonZeroInputsProduceNonZero)
{
    for (bee::u64 i = 1; i <= 50; ++i)
        EXPECT_NE(bee::Splitmix64(i), bee::u64{0});
}

TEST(BaseHashTests, Splitmix64AvalancheSingleBitFlip)
{
    auto popcount64 = [](bee::u64 x) -> int {
        int count = 0;
        while (x) {
            count += static_cast<int>(x & 1);
            x     >>= 1;
        }
        return count;
    };

    constexpr bee::u64 bases[] = {1, 42, 0x123456789ABCDEF0ULL, 1000, UINT64_MAX / 2};
    constexpr int bits[]       = {0, 1, 16, 31, 63};

    int total_differing = 0;
    int test_count      = 0;

    for (auto base : bases) {
        const auto h0 = bee::Splitmix64(base);
        for (auto bit : bits) {
            const auto h1   = bee::Splitmix64(base ^ (bee::u64{1} << bit));
            const int diff  = popcount64(h0 ^ h1);
            total_differing += diff;
            ++test_count;
            EXPECT_GE(diff, 10) << "base=0x" << std::hex << base << " bit=" << bit;
        }
    }

    const double avg = static_cast<double>(total_differing) / test_count;
    EXPECT_GE(avg, 20.0);
}
