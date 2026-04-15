#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

#include "Base/Base.hpp"
#include "Base/Core/Config.hpp"
#include "Base/Numeric/Hash.hpp"
#include "Base/Core/Macros.hpp"
#include "Base/Numeric/Numeric.hpp"

TEST(BaseComponentTests, ReturnsComponentName)
{
    EXPECT_EQ(bee::base_name(), "Base");
}

// ── Hash.hpp ────────────────────────────────────────────────────────────────────

TEST(BaseHashTests, Splitmix64ZeroInput)
{
    // Splitmix64 mixing function: 0 input produces 0 (all XOR+multiply from zero)
    EXPECT_EQ(bee::Splitmix64(0), 0u);
}

TEST(BaseHashTests, DifferentInputsProduceDifferentOutputs)
{
    EXPECT_NE(bee::Splitmix64(0), bee::Splitmix64(1));
    EXPECT_NE(bee::Splitmix64(1), bee::Splitmix64(2));
    EXPECT_NE(bee::Splitmix64(100), bee::Splitmix64(200));
}

TEST(BaseHashTests, ConstexprEvaluation)
{
    constexpr auto h = bee::Splitmix64(42);
    EXPECT_NE(h, 0u);
}

TEST(BaseHashTests, NoCollisionsForSmallInputRange)
{
    std::set<bee::u64> results;
    for (bee::u64 i = 0; i < 256; ++i) {
        results.insert(bee::Splitmix64(i));
    }
    EXPECT_EQ(results.size(), 256u);
}

// ── Numeric.hpp ─────────────────────────────────────────────────────────────────

TEST(BaseNumericTests, NormalAddition)
{
    EXPECT_EQ(bee::SaturatingAdd(3u, 4u), 7u);
}

TEST(BaseNumericTests, SaturatesAtMaxPlusOne)
{
    EXPECT_EQ(bee::SaturatingAdd(UINT32_MAX, 1u), UINT32_MAX);
}

TEST(BaseNumericTests, SaturatesAtMaxPlusMax)
{
    EXPECT_EQ(bee::SaturatingAdd(UINT32_MAX, UINT32_MAX), UINT32_MAX);
}

TEST(BaseNumericTests, NearMaxDoesNotSaturate)
{
    EXPECT_EQ(bee::SaturatingAdd(UINT32_MAX - 5u, 3u), UINT32_MAX - 2u);
}

TEST(BaseNumericTests, ZeroPlusZero)
{
    EXPECT_EQ(bee::SaturatingAdd(0u, 0u), 0u);
}

TEST(BaseNumericTests, WorksWithUint8)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint8_t>(200, 100), std::numeric_limits<std::uint8_t>::max());
    EXPECT_EQ(bee::SaturatingAdd<std::uint8_t>(10, 20), static_cast<std::uint8_t>(30));
}

TEST(BaseNumericTests, WorksWithUint16)
{
    EXPECT_EQ(bee::SaturatingAdd<std::uint16_t>(60000, 10000), std::numeric_limits<std::uint16_t>::max());
    EXPECT_EQ(bee::SaturatingAdd<std::uint16_t>(100, 200), static_cast<std::uint16_t>(300));
}

TEST(BaseNumericTests, WorksWithUint64)
{
    EXPECT_EQ(bee::SaturatingAdd(UINT64_MAX, static_cast<std::uint64_t>(1)), UINT64_MAX);
    EXPECT_EQ(bee::SaturatingAdd(static_cast<std::uint64_t>(10), static_cast<std::uint64_t>(20)), static_cast<std::uint64_t>(30));
}

// ── Macros.hpp ──────────────────────────────────────────────────────────────────

TEST(BaseMacrosTests, IntArraySize)
{
    int arr[5] = {};
    EXPECT_EQ(BEE_ARRAY_SIZE(arr), 5u);
}

TEST(BaseMacrosTests, CharArraySize)
{
    char arr[12] = {};
    EXPECT_EQ(BEE_ARRAY_SIZE(arr), 12u);
}

TEST(BaseMacrosTests, SingleElementArray)
{
    double arr[1] = {};
    EXPECT_EQ(BEE_ARRAY_SIZE(arr), 1u);
}

TEST(BaseMacrosTests, StructArray)
{
    struct S
    {
        int   x;
        float y;
    };
    S arr[7] = {};
    EXPECT_EQ(BEE_ARRAY_SIZE(arr), 7u);
}

// ── Config.hpp ──────────────────────────────────────────────────────────────────

TEST(BaseConfigTests, SignedTypeSizes)
{
    EXPECT_EQ(sizeof(bee::i8), 1u);
    EXPECT_EQ(sizeof(bee::i16), 2u);
    EXPECT_EQ(sizeof(bee::i32), 4u);
    EXPECT_EQ(sizeof(bee::i64), 8u);
}

TEST(BaseConfigTests, UnsignedTypeSizes)
{
    EXPECT_EQ(sizeof(bee::u8), 1u);
    EXPECT_EQ(sizeof(bee::u16), 2u);
    EXPECT_EQ(sizeof(bee::u32), 4u);
    EXPECT_EQ(sizeof(bee::u64), 8u);
}

TEST(BaseConfigTests, SignedTypesAreSigned)
{
    EXPECT_TRUE(std::is_signed_v<bee::i8>);
    EXPECT_TRUE(std::is_signed_v<bee::i16>);
    EXPECT_TRUE(std::is_signed_v<bee::i32>);
    EXPECT_TRUE(std::is_signed_v<bee::i64>);
}

TEST(BaseConfigTests, UnsignedTypesAreUnsigned)
{
    EXPECT_TRUE(std::is_unsigned_v<bee::u8>);
    EXPECT_TRUE(std::is_unsigned_v<bee::u16>);
    EXPECT_TRUE(std::is_unsigned_v<bee::u32>);
    EXPECT_TRUE(std::is_unsigned_v<bee::u64>);
}
