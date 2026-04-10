/**
 * @File EnumTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/23
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

#include "Base/Reflection/Enum.hpp"

namespace bee::enum_test_samples
{

enum class Permission : std::uint8_t
{
    None    = 0,
    Read    = 1,
    Write   = 2,
    Execute = 4
};

enum class Feature : std::uint32_t
{
    None = 0,
    A    = 1u << 0u,
    B    = 1u << 1u,
    C    = 1u << 2u,
    High = 1u << 31u
};

enum class Plain : std::uint8_t
{
    A = 1,
    B = 2
};

} // namespace bee::enum_test_samples

BEE_ENABLE_ENUM_BITMASK_OPERATORS(bee::enum_test_samples::Permission);
BEE_ENABLE_ENUM_BITMASK_OPERATORS(bee::enum_test_samples::Feature);

TEST(EnumBitmaskTests, SupportsBitwiseOperators)
{
    const auto value = bee::enum_test_samples::Permission::Read | bee::enum_test_samples::Permission::Write;
    EXPECT_TRUE(bee::EnumHasAny(value, bee::enum_test_samples::Permission::Read));
    EXPECT_TRUE(bee::EnumHasAny(value, bee::enum_test_samples::Permission::Write));
    EXPECT_FALSE(bee::EnumHasAny(value, bee::enum_test_samples::Permission::Execute));
}

TEST(EnumBitmaskTests, EnablesBitmaskByMacro)
{
    static_assert(bee::EnumBitmask<bee::enum_test_samples::Permission>);
    static_assert(bee::EnumBitmask<bee::enum_test_samples::Feature>);
    static_assert(!bee::EnumBitmask<bee::enum_test_samples::Plain>);
}

TEST(EnumBitmaskTests, SupportsCompoundAssignmentOperators)
{
    auto value = bee::enum_test_samples::Permission::Read;
    value |= bee::enum_test_samples::Permission::Write;
    EXPECT_TRUE(bee::EnumHasAll(value, bee::enum_test_samples::Permission::Read | bee::enum_test_samples::Permission::Write));

    value &= bee::enum_test_samples::Permission::Write;
    EXPECT_EQ(value, bee::enum_test_samples::Permission::Write);

    value ^= bee::enum_test_samples::Permission::Execute;
    EXPECT_TRUE(bee::EnumHasAll(value, bee::enum_test_samples::Permission::Write | bee::enum_test_samples::Permission::Execute));
}

TEST(EnumBitmaskTests, SupportsSetClearToggle)
{
    auto value = bee::enum_test_samples::Permission::None;
    value = bee::EnumSet(value, bee::enum_test_samples::Permission::Read);
    EXPECT_EQ(value, bee::enum_test_samples::Permission::Read);

    value = bee::EnumToggle(value, bee::enum_test_samples::Permission::Write);
    EXPECT_TRUE(bee::EnumHasAll(value, bee::enum_test_samples::Permission::Read | bee::enum_test_samples::Permission::Write));

    value = bee::EnumClear(value, bee::enum_test_samples::Permission::Read);
    EXPECT_EQ(value, bee::enum_test_samples::Permission::Write);
}

TEST(EnumBitmaskTests, ClearAbsentFlagKeepsValue)
{
    const auto value   = bee::enum_test_samples::Permission::Read;
    const auto cleared = bee::EnumClear(value, bee::enum_test_samples::Permission::Execute);
    EXPECT_EQ(cleared, value);
}

TEST(EnumBitmaskTests, DetectsHasAllHasAnyHasNone)
{
    const auto value = bee::enum_test_samples::Permission::Read | bee::enum_test_samples::Permission::Execute;
    EXPECT_TRUE(bee::EnumHasAll(value, bee::enum_test_samples::Permission::Read));
    EXPECT_TRUE(bee::EnumHasAny(value, bee::enum_test_samples::Permission::Execute));
    EXPECT_TRUE(bee::EnumHasNone(value, bee::enum_test_samples::Permission::Write));
    EXPECT_TRUE(bee::EnumHasAll(value, bee::enum_test_samples::Permission::None));
}

TEST(EnumBitmaskTests, DetectsZeroAndSingleBit)
{
    EXPECT_TRUE(bee::EnumIsZero(bee::enum_test_samples::Permission::None));
    EXPECT_TRUE(bee::EnumIsSingleBit(bee::enum_test_samples::Permission::Read));
    EXPECT_FALSE(bee::EnumIsSingleBit(bee::enum_test_samples::Permission::Read | bee::enum_test_samples::Permission::Write));
    EXPECT_EQ(bee::EnumBitCount(bee::enum_test_samples::Permission::None), 0);
}

TEST(EnumBitmaskTests, CountsEnabledBits)
{
    const auto value = bee::enum_test_samples::Permission::Read | bee::enum_test_samples::Permission::Write |
                       bee::enum_test_samples::Permission::Execute;
    EXPECT_EQ(bee::EnumBitCount(value), 3);
}

TEST(EnumBitmaskTests, ConvertsToUnderlying)
{
    static_assert(std::is_same_v<decltype(bee::ToUnderlying(bee::enum_test_samples::Permission::Read)),
                                 std::underlying_type_t<bee::enum_test_samples::Permission>>);
    EXPECT_EQ(bee::ToUnderlying(bee::enum_test_samples::Permission::Execute), 4);
}

TEST(EnumBitmaskTests, SupportsAndNotOperators)
{
    const auto value = bee::enum_test_samples::Permission::Read | bee::enum_test_samples::Permission::Write;
    EXPECT_EQ((value & bee::enum_test_samples::Permission::Read), bee::enum_test_samples::Permission::Read);
    EXPECT_EQ((value & ~bee::enum_test_samples::Permission::Read), bee::enum_test_samples::Permission::Write);
}

TEST(EnumBitmaskTests, HandlesHighBitForUnsignedUnderlying)
{
    const auto value = bee::enum_test_samples::Feature::High | bee::enum_test_samples::Feature::A;
    EXPECT_TRUE(bee::EnumHasAll(value, bee::enum_test_samples::Feature::High));
    EXPECT_TRUE(bee::EnumHasAny(value, bee::enum_test_samples::Feature::A));
    EXPECT_EQ(bee::EnumBitCount(value), 2);
}
