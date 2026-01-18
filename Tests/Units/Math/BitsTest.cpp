/**
 * @File BitsTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee/Math/Bits.hpp>

using namespace bee;

TEST(BitsTest, BitCast)
{
    // 测试 f32 到 u32 的转换
    f32 f  = 3.14159f;
    u32 ui = BitCast<u32>(f);
    f32 f2 = BitCast<f32>(ui);
    EXPECT_EQ(f, f2);

    // 测试 f64 到 u64 的转换
    f64 d  = 2.718281828;
    u64 ul = BitCast<u64>(d);
    f64 d2 = BitCast<f64>(ul);
    EXPECT_EQ(d, d2);

    // 测试整数转换
    i32 i = 0x12345678;
    u32 u = BitCast<u32>(i);
    EXPECT_EQ(static_cast<u32>(i), u);
}

TEST(BitsTest, HasSingleBit)
{
    // 测试 2 的幂
    EXPECT_TRUE(HasSingleBit(1u));
    EXPECT_TRUE(HasSingleBit(2u));
    EXPECT_TRUE(HasSingleBit(4u));
    EXPECT_TRUE(HasSingleBit(8u));
    EXPECT_TRUE(HasSingleBit(16u));
    EXPECT_TRUE(HasSingleBit(1024u));

    // 测试非 2 的幂
    EXPECT_FALSE(HasSingleBit(0u));
    EXPECT_FALSE(HasSingleBit(3u));
    EXPECT_FALSE(HasSingleBit(5u));
    EXPECT_FALSE(HasSingleBit(6u));
    EXPECT_FALSE(HasSingleBit(7u));
    EXPECT_FALSE(HasSingleBit(15u));

    // 测试边界值
    EXPECT_TRUE(HasSingleBit(static_cast<u32>(1) << 31));
    EXPECT_TRUE(HasSingleBit(static_cast<u64>(1) << 63));
}

TEST(BitsTest, BitCeil)
{
    EXPECT_EQ(BitCeil(0u), 1u);
    EXPECT_EQ(BitCeil(1u), 1u);
    EXPECT_EQ(BitCeil(2u), 2u);
    EXPECT_EQ(BitCeil(3u), 4u);
    EXPECT_EQ(BitCeil(4u), 4u);
    EXPECT_EQ(BitCeil(5u), 8u);
    EXPECT_EQ(BitCeil(7u), 8u);
    EXPECT_EQ(BitCeil(8u), 8u);
    EXPECT_EQ(BitCeil(9u), 16u);
    EXPECT_EQ(BitCeil(15u), 16u);
    EXPECT_EQ(BitCeil(16u), 16u);
    EXPECT_EQ(BitCeil(17u), 32u);
}

TEST(BitsTest, BitFloor)
{
    EXPECT_EQ(BitFloor(0u), 0u);
    EXPECT_EQ(BitFloor(1u), 1u);
    EXPECT_EQ(BitFloor(2u), 2u);
    EXPECT_EQ(BitFloor(3u), 2u);
    EXPECT_EQ(BitFloor(4u), 4u);
    EXPECT_EQ(BitFloor(5u), 4u);
    EXPECT_EQ(BitFloor(7u), 4u);
    EXPECT_EQ(BitFloor(8u), 8u);
    EXPECT_EQ(BitFloor(9u), 8u);
    EXPECT_EQ(BitFloor(15u), 8u);
    EXPECT_EQ(BitFloor(16u), 16u);
    EXPECT_EQ(BitFloor(17u), 16u);
}

TEST(BitsTest, BitWidth)
{
    EXPECT_EQ(BitWidth(0u), 0u);
    EXPECT_EQ(BitWidth(1u), 1u);
    EXPECT_EQ(BitWidth(2u), 2u);
    EXPECT_EQ(BitWidth(3u), 2u);
    EXPECT_EQ(BitWidth(4u), 3u);
    EXPECT_EQ(BitWidth(7u), 3u);
    EXPECT_EQ(BitWidth(8u), 4u);
    EXPECT_EQ(BitWidth(15u), 4u);
    EXPECT_EQ(BitWidth(16u), 5u);
    EXPECT_EQ(BitWidth(255u), 8u);
    EXPECT_EQ(BitWidth(256u), 9u);
}

TEST(BitsTest, RotateLeft)
{
    uint8_t val8 = 0b10110001;
    EXPECT_EQ(RotateLeft(val8, 1), static_cast<uint8_t>(0b01100011));
    EXPECT_EQ(RotateLeft(val8, 2), static_cast<uint8_t>(0b11000110));
    EXPECT_EQ(RotateLeft(val8, 8), val8); // 完整旋转

    uint16_t val16 = 0x1234;
    EXPECT_EQ(RotateLeft(val16, 4), static_cast<uint16_t>(0x2341));
    EXPECT_EQ(RotateLeft(val16, 8), static_cast<uint16_t>(0x3412));

    u32 val32 = 0x12345678;
    EXPECT_EQ(RotateLeft(val32, 8), 0x34567812u);
    EXPECT_EQ(RotateLeft(val32, 16), 0x56781234u);
}

TEST(BitsTest, RotateRight)
{
    uint8_t val8 = 0b10110001;
    EXPECT_EQ(RotateRight(val8, 1), static_cast<uint8_t>(0b11011000));
    EXPECT_EQ(RotateRight(val8, 2), static_cast<uint8_t>(0b01101100));
    EXPECT_EQ(RotateRight(val8, 8), val8); // 完整旋转

    u32 val32 = 0x12345678;
    EXPECT_EQ(RotateRight(val32, 8), 0x78123456u);
    EXPECT_EQ(RotateRight(val32, 16), 0x56781234u);
}

TEST(BitsTest, CountLeadingZero)
{
    EXPECT_EQ(CountLeadingZero(static_cast<uint8_t>(0)), 8u);
    EXPECT_EQ(CountLeadingZero(static_cast<uint8_t>(1)), 7u);
    EXPECT_EQ(CountLeadingZero(static_cast<uint8_t>(0x80)), 0u);
    EXPECT_EQ(CountLeadingZero(static_cast<uint8_t>(0x40)), 1u);

    EXPECT_EQ(CountLeadingZero(static_cast<u32>(0)), 32u);
    EXPECT_EQ(CountLeadingZero(static_cast<u32>(1)), 31u);
    EXPECT_EQ(CountLeadingZero(static_cast<u32>(0x80000000)), 0u);
    EXPECT_EQ(CountLeadingZero(static_cast<u32>(0x40000000)), 1u);
}

TEST(BitsTest, CountLeadingOne)
{
    EXPECT_EQ(CountLeadingOne(static_cast<uint8_t>(0xFF)), 8u);
    EXPECT_EQ(CountLeadingOne(static_cast<uint8_t>(0xFE)), 7u);
    EXPECT_EQ(CountLeadingOne(static_cast<uint8_t>(0x7F)), 0u);
    EXPECT_EQ(CountLeadingOne(static_cast<uint8_t>(0)), 0u);

    EXPECT_EQ(CountLeadingOne(static_cast<u32>(0xFFFFFFFF)), 32u);
    EXPECT_EQ(CountLeadingOne(static_cast<u32>(0xFFFFFFFE)), 31u);
    EXPECT_EQ(CountLeadingOne(static_cast<u32>(0x7FFFFFFF)), 0u);
}

TEST(BitsTest, CountTrailingZero)
{
    EXPECT_EQ(CountTrailingZero(static_cast<uint8_t>(0)), 8u);
    EXPECT_EQ(CountTrailingZero(static_cast<uint8_t>(1)), 0u);
    EXPECT_EQ(CountTrailingZero(static_cast<uint8_t>(2)), 1u);
    EXPECT_EQ(CountTrailingZero(static_cast<uint8_t>(4)), 2u);
    EXPECT_EQ(CountTrailingZero(static_cast<uint8_t>(8)), 3u);

    EXPECT_EQ(CountTrailingZero(static_cast<u32>(0)), 32u);
    EXPECT_EQ(CountTrailingZero(static_cast<u32>(0x80000000)), 31u);
}

TEST(BitsTest, CountTrailingOne)
{
    EXPECT_EQ(CountTrailingOne(static_cast<uint8_t>(0xFF)), 8u);
    EXPECT_EQ(CountTrailingOne(static_cast<uint8_t>(0x7F)), 7u);
    EXPECT_EQ(CountTrailingOne(static_cast<uint8_t>(0x3F)), 6u);
    EXPECT_EQ(CountTrailingOne(static_cast<uint8_t>(0)), 0u);
    EXPECT_EQ(CountTrailingOne(static_cast<uint8_t>(2)), 0u);

    EXPECT_EQ(CountTrailingOne(static_cast<u32>(0xFFFFFFFF)), 32u);
    EXPECT_EQ(CountTrailingOne(static_cast<u32>(0x7FFFFFFF)), 31u);
}

TEST(BitsTest, OneBitCount)
{
    EXPECT_EQ(OneBitCount(static_cast<uint8_t>(0)), 0);
    EXPECT_EQ(OneBitCount(static_cast<uint8_t>(1)), 1);
    EXPECT_EQ(OneBitCount(static_cast<uint8_t>(3)), 2);
    EXPECT_EQ(OneBitCount(static_cast<uint8_t>(7)), 3);
    EXPECT_EQ(OneBitCount(static_cast<uint8_t>(15)), 4);
    EXPECT_EQ(OneBitCount(static_cast<uint8_t>(0xFF)), 8);

    EXPECT_EQ(OneBitCount(static_cast<u32>(0)), 0);
    EXPECT_EQ(OneBitCount(static_cast<u32>(0xFFFFFFFF)), 32);
    EXPECT_EQ(OneBitCount(static_cast<u32>(0x12345678)), 13);
}

TEST(BitsTest, ZeroBitCount)
{
    EXPECT_EQ(ZeroBitCount(static_cast<uint8_t>(0)), 8);
    EXPECT_EQ(ZeroBitCount(static_cast<uint8_t>(1)), 7);
    EXPECT_EQ(ZeroBitCount(static_cast<uint8_t>(3)), 6);
    EXPECT_EQ(ZeroBitCount(static_cast<uint8_t>(7)), 5);
    EXPECT_EQ(ZeroBitCount(static_cast<uint8_t>(15)), 4);
    EXPECT_EQ(ZeroBitCount(static_cast<uint8_t>(0xFF)), 0);

    EXPECT_EQ(ZeroBitCount(static_cast<u32>(0)), 32);
    EXPECT_EQ(ZeroBitCount(static_cast<u32>(0xFFFFFFFF)), 0);
    EXPECT_EQ(ZeroBitCount(static_cast<u32>(0x12345678)), 19);
}

TEST(BitsTest, IsPowerOfTwo)
{
    EXPECT_FALSE(IsPowerOfTwo(0u));
    EXPECT_TRUE(IsPowerOfTwo(1u));
    EXPECT_TRUE(IsPowerOfTwo(2u));
    EXPECT_FALSE(IsPowerOfTwo(3u));
    EXPECT_TRUE(IsPowerOfTwo(4u));
    EXPECT_FALSE(IsPowerOfTwo(5u));
    EXPECT_FALSE(IsPowerOfTwo(6u));
    EXPECT_FALSE(IsPowerOfTwo(7u));
    EXPECT_TRUE(IsPowerOfTwo(8u));
    EXPECT_TRUE(IsPowerOfTwo(1024u));
    EXPECT_FALSE(IsPowerOfTwo(1023u));
    EXPECT_FALSE(IsPowerOfTwo(1025u));
}

TEST(BitsTest, PreviousPowerOfTwo)
{
    EXPECT_EQ(PreviousPowerOfTwo(0u), 0u);
    EXPECT_EQ(PreviousPowerOfTwo(1u), 1u);
    EXPECT_EQ(PreviousPowerOfTwo(2u), 2u);
    EXPECT_EQ(PreviousPowerOfTwo(3u), 2u);
    EXPECT_EQ(PreviousPowerOfTwo(4u), 4u);
    EXPECT_EQ(PreviousPowerOfTwo(5u), 4u);
    EXPECT_EQ(PreviousPowerOfTwo(7u), 4u);
    EXPECT_EQ(PreviousPowerOfTwo(8u), 8u);
    EXPECT_EQ(PreviousPowerOfTwo(15u), 8u);
    EXPECT_EQ(PreviousPowerOfTwo(16u), 16u);
    EXPECT_EQ(PreviousPowerOfTwo(17u), 16u);
}

TEST(BitsTest, NextPowerOfTwo)
{
    EXPECT_EQ(NextPowerOfTwo(0u), 1u);
    EXPECT_EQ(NextPowerOfTwo(1u), 1u);
    EXPECT_EQ(NextPowerOfTwo(2u), 2u);
    EXPECT_EQ(NextPowerOfTwo(3u), 4u);
    EXPECT_EQ(NextPowerOfTwo(4u), 4u);
    EXPECT_EQ(NextPowerOfTwo(5u), 8u);
    EXPECT_EQ(NextPowerOfTwo(7u), 8u);
    EXPECT_EQ(NextPowerOfTwo(8u), 8u);
    EXPECT_EQ(NextPowerOfTwo(9u), 16u);
    EXPECT_EQ(NextPowerOfTwo(15u), 16u);
    EXPECT_EQ(NextPowerOfTwo(16u), 16u);
    EXPECT_EQ(NextPowerOfTwo(17u), 32u);
}

TEST(BitsTest, ClosestPowerOfTwo)
{
    EXPECT_EQ(ClosestPowerOfTwo(0u), 1u);
    EXPECT_EQ(ClosestPowerOfTwo(1u), 1u);
    EXPECT_EQ(ClosestPowerOfTwo(2u), 2u);
    EXPECT_EQ(ClosestPowerOfTwo(3u), 2u); // 距离 2 和 4 相等，选择较小的
    EXPECT_EQ(ClosestPowerOfTwo(4u), 4u);
    EXPECT_EQ(ClosestPowerOfTwo(5u), 4u);
    EXPECT_EQ(ClosestPowerOfTwo(6u), 4u);
    EXPECT_EQ(ClosestPowerOfTwo(7u), 8u);
    EXPECT_EQ(ClosestPowerOfTwo(8u), 8u);
    EXPECT_EQ(ClosestPowerOfTwo(10u), 8u);
    EXPECT_EQ(ClosestPowerOfTwo(13u), 16u);
}

TEST(BitsTest, ReverseBits)
{
    // 8-bit 测试
    EXPECT_EQ(ReverseBits(static_cast<uint8_t>(0b10110001)), static_cast<uint8_t>(0b10001101));
    EXPECT_EQ(ReverseBits(static_cast<uint8_t>(0b11110000)), static_cast<uint8_t>(0b00001111));
    EXPECT_EQ(ReverseBits(static_cast<uint8_t>(0)), static_cast<uint8_t>(0));
    EXPECT_EQ(ReverseBits(static_cast<uint8_t>(0xFF)), static_cast<uint8_t>(0xFF));

    // 16-bit 测试
    EXPECT_EQ(ReverseBits(static_cast<uint16_t>(0x1234)), static_cast<uint16_t>(0x2C48));
    EXPECT_EQ(ReverseBits(static_cast<uint16_t>(0xF0F0)), static_cast<uint16_t>(0x0F0F));

    // 32-bit 测试
    EXPECT_EQ(ReverseBits(static_cast<u32>(0x12345678)), 0x1E6A2C48u);
    EXPECT_EQ(ReverseBits(static_cast<u32>(0)), 0u);
    EXPECT_EQ(ReverseBits(static_cast<u32>(0xFFFFFFFF)), 0xFFFFFFFFu);
}

TEST(BitsTest, SetBit)
{
    u32 value = 0;
    EXPECT_EQ(SetBit(value, 0), 1u);
    EXPECT_EQ(SetBit(value, 1), 2u);
    EXPECT_EQ(SetBit(value, 2), 4u);
    EXPECT_EQ(SetBit(value, 31), 0x80000000u);

    value = 0x12345678;
    EXPECT_EQ(SetBit(value, 0), 0x12345679u);
    EXPECT_EQ(SetBit(value, 7), 0x123456F8u);
}

TEST(BitsTest, ClearBit)
{
    u32 value = 0xFFFFFFFF;
    EXPECT_EQ(ClearBit(value, 0), 0xFFFFFFFEu);
    EXPECT_EQ(ClearBit(value, 1), 0xFFFFFFFDu);
    EXPECT_EQ(ClearBit(value, 31), 0x7FFFFFFFu);

    value = 0x12345678;
    EXPECT_EQ(ClearBit(value, 3), 0x12345670u);
    EXPECT_EQ(ClearBit(value, 6), 0x12345638u);
}

TEST(BitsTest, ToggleBit)
{
    u32 value = 0;
    EXPECT_EQ(ToggleBit(value, 0), 1u);
    EXPECT_EQ(ToggleBit(value, 1), 2u);

    value = 0xFFFFFFFF;
    EXPECT_EQ(ToggleBit(value, 0), 0xFFFFFFFEu);
    EXPECT_EQ(ToggleBit(value, 31), 0x7FFFFFFFu);

    value = 0x12345678;
    EXPECT_EQ(ToggleBit(value, 0), 0x12345679u);
    EXPECT_EQ(ToggleBit(value, 3), 0x12345670u);
}

TEST(BitsTest, CheckBit)
{
    u32 value = 0x12345678;
    EXPECT_FALSE(CheckBit(value, 0));
    EXPECT_FALSE(CheckBit(value, 1));
    EXPECT_FALSE(CheckBit(value, 2));
    EXPECT_TRUE(CheckBit(value, 3));
    EXPECT_TRUE(CheckBit(value, 4));
    EXPECT_FALSE(CheckBit(value, 7));

    EXPECT_FALSE(CheckBit(0u, 0));
    EXPECT_TRUE(CheckBit(0xFFFFFFFFu, 31));
}

TEST(BitsTest, RoundUp)
{
    EXPECT_EQ(RoundUp(0, 4), 0u);
    EXPECT_EQ(RoundUp(1, 4), 4u);
    EXPECT_EQ(RoundUp(4, 4), 4u);
    EXPECT_EQ(RoundUp(5, 4), 8u);
    EXPECT_EQ(RoundUp(7, 4), 8u);
    EXPECT_EQ(RoundUp(8, 4), 8u);
    EXPECT_EQ(RoundUp(9, 4), 12u);

    EXPECT_EQ(RoundUp(10, 8), 16u);
    EXPECT_EQ(RoundUp(16, 8), 16u);
    EXPECT_EQ(RoundUp(17, 8), 24u);

    // y == 0 时返回 x
    EXPECT_EQ(RoundUp(5, 0), 5u);
}

TEST(BitsTest, AlignUp)
{
    // 测试 2 的幂对齐
    EXPECT_EQ(AlignUp(0, 4), 0u);
    EXPECT_EQ(AlignUp(1, 4), 4u);
    EXPECT_EQ(AlignUp(4, 4), 4u);
    EXPECT_EQ(AlignUp(5, 4), 8u);
    EXPECT_EQ(AlignUp(7, 4), 8u);
    EXPECT_EQ(AlignUp(8, 4), 8u);

    EXPECT_EQ(AlignUp(15, 16), 16u);
    EXPECT_EQ(AlignUp(16, 16), 16u);
    EXPECT_EQ(AlignUp(17, 16), 32u);

    // 测试更大的对齐值
    EXPECT_EQ(AlignUp(100, 64), 128u);
    EXPECT_EQ(AlignUp(128, 64), 128u);
    EXPECT_EQ(AlignUp(129, 64), 192u);
}

TEST(BitsTest, EdgeCases)
{
    // 测试 0 值
    EXPECT_EQ(BitCeil(0u), 1u);
    EXPECT_EQ(BitFloor(0u), 0u);
    EXPECT_EQ(BitWidth(0u), 0u);
    EXPECT_FALSE(IsPowerOfTwo(0u));
    EXPECT_FALSE(IsPowerOfTwo(0u));

    // 测试最大值
    constexpr u32 max32 = std::numeric_limits<u32>::max();
    EXPECT_EQ(CountLeadingZero(max32), 0u);
    EXPECT_EQ(CountTrailingOne(max32), 32u);
    EXPECT_EQ(OneBitCount(max32), 32);

    constexpr u64 max64 = std::numeric_limits<u64>::max();
    EXPECT_EQ(CountLeadingZero(max64), 0u);
    EXPECT_EQ(CountTrailingOne(max64), 64u);
    EXPECT_EQ(OneBitCount(max64), 64);

    // 测试单个位设置
    for (int i = 0; i < 32; ++i)
    {
        u32 val = 1u << i;
        EXPECT_TRUE(IsPowerOfTwo(val));
        EXPECT_TRUE(IsPowerOfTwo(val));
        EXPECT_EQ(OneBitCount(val), 1);
        EXPECT_EQ(CountTrailingZero(val), static_cast<u32>(i));
        EXPECT_EQ(CountLeadingZero(val), static_cast<u32>(31 - i));
    }
}

TEST(BitsTest, TypeConsistency)
{
    // 测试不同类型的一致性
    uint8_t val8   = 0x0F;
    uint16_t val16 = 0x0F;
    u32 val32      = 0x0F;
    u64 val64      = 0x0F;

    EXPECT_EQ(OneBitCount(val8), OneBitCount(val16));
    EXPECT_EQ(OneBitCount(val16), OneBitCount(val32));
    EXPECT_EQ(OneBitCount(val32), OneBitCount(val64));

    EXPECT_TRUE(IsPowerOfTwo(static_cast<uint8_t>(8)));
    EXPECT_TRUE(IsPowerOfTwo(static_cast<uint16_t>(8)));
    EXPECT_TRUE(IsPowerOfTwo(static_cast<u32>(8)));
    EXPECT_TRUE(IsPowerOfTwo(static_cast<u64>(8)));
}
