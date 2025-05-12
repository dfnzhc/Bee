/**
 * @File MathTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/15
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee.hpp>

#include <Core/Math/Bits.hpp>

using namespace bee;

// Test fixture for typed tests, allowing tests for uint8_t, uint16_t, etc.
template<typename T>
class BitOpsTest : public ::testing::Test
{
protected:
    static constexpr T zero_val    = static_cast<T>(0);
    static constexpr T one_val     = static_cast<T>(1);
    static constexpr T max_val     = std::numeric_limits<T>::max();
    static constexpr int bits_in_T = detail::BitSize<T>();
};

// Define the types to be tested
using UnsignedTypes = ::testing::Types<u8, u16, u32, u64>;
TYPED_TEST_SUITE(BitOpsTest, UnsignedTypes);

TYPED_TEST(BitOpsTest, CountLeadingZeros)
{
    using T = TypeParam;
    EXPECT_EQ(CountLeadingZeros(this->zero_val), this->bits_in_T);
    EXPECT_EQ(CountLeadingZeros(this->one_val), this->bits_in_T - 1);
    if (this->bits_in_T > 1) {
        EXPECT_EQ(CountLeadingZeros<T>(static_cast<T>(1) << (this->bits_in_T - 1)), 0);
        EXPECT_EQ(CountLeadingZeros<T>(static_cast<T>(3) << (this->bits_in_T - 2)), 0); // e.g. 0b11...
    }
    EXPECT_EQ(CountLeadingZeros(this->max_val), 0);
    if (this->bits_in_T >= 8) {
        // Test a specific pattern if enough bits
        T val = static_cast<T>(0x0F) << (this->bits_in_T - 8); // e.g. 0x0F000000 for uint32
        if (this->bits_in_T > 4)                               // ensure leading zeros before 0F part
            EXPECT_EQ(CountLeadingZeros(val), 4);              // clz for 0F....
    }
}

TYPED_TEST(BitOpsTest, CountTrailingZeros)
{
    using T = TypeParam;
    EXPECT_EQ(CountTrailingZeros(this->zero_val), this->bits_in_T);
    EXPECT_EQ(CountTrailingZeros(this->one_val), 0);
    if (this->bits_in_T > 1) {
        EXPECT_EQ(CountTrailingZeros<T>(static_cast<T>(1) << (this->bits_in_T - 1)), this->bits_in_T - 1);
    }
    if (this->bits_in_T > 0) {
        // max_val is all 1s if T is not empty
        EXPECT_EQ(CountTrailingZeros(this->max_val), 0);
    }
    T val_ending_zeros = static_cast<T>(0xFE); // ...11111110
    if (this->bits_in_T >= 8) {
        // Example with more bits
        val_ending_zeros = static_cast<T>(0xF0); // ...11110000
        EXPECT_EQ(CountTrailingZeros(val_ending_zeros), 4);
    }
    else if (this->bits_in_T > 0) {
        EXPECT_EQ(CountTrailingZeros(static_cast<T>(~this->one_val)), 1); // e.g., for uint8_t, 0xFE
    }
}

TYPED_TEST(BitOpsTest, Popcount)
{
    using T = TypeParam;
    EXPECT_EQ(Popcount(this->zero_val), 0);
    EXPECT_EQ(Popcount(this->one_val), 1);
    EXPECT_EQ(Popcount(this->max_val), this->bits_in_T);
    EXPECT_EQ(Popcount(static_cast<T>(0x0F0F0F0F0F0F0F0F)), this->bits_in_T / 2);
    EXPECT_EQ(Popcount(static_cast<T>(0x5555555555555555)), this->bits_in_T / 2); // 01010101
    EXPECT_EQ(Popcount(static_cast<T>(0xAAAAAAAAAAAAAAAA)), this->bits_in_T / 2); // 10101010
}

TYPED_TEST(BitOpsTest, CountClearedBits)
{
    using T = TypeParam;
    EXPECT_EQ(CountClearedBits(this->zero_val), this->bits_in_T);
    EXPECT_EQ(CountClearedBits(this->one_val), this->bits_in_T - 1);
    EXPECT_EQ(CountClearedBits(this->max_val), 0);
}

TYPED_TEST(BitOpsTest, Parity)
{
    using T = TypeParam;
    EXPECT_EQ(Parity(this->zero_val), 0);
    EXPECT_EQ(Parity(this->one_val), 1);
    EXPECT_EQ(Parity(this->max_val), this->bits_in_T % 2);
    if (this->bits_in_T >= 2) {
        EXPECT_EQ(Parity(static_cast<T>(3)), 0); // 0b11
    }
    if (this->bits_in_T >= 3) {
        EXPECT_EQ(Parity(static_cast<T>(7)), 1); // 0b111
    }
}

TYPED_TEST(BitOpsTest, HasSingleBitAndIsPowerOfTwo)
{
    using T = TypeParam;
    EXPECT_FALSE(HasSingleBit(this->zero_val));
    EXPECT_TRUE(HasSingleBit(this->one_val));
    EXPECT_FALSE(IsPowerOfTwo(this->zero_val));
    EXPECT_TRUE(IsPowerOfTwo(this->one_val));

    for (int i = 0; i < this->bits_in_T; ++i) {
        T power_of_2 = static_cast<T>(1) << i;
        EXPECT_TRUE(HasSingleBit(power_of_2));
        EXPECT_TRUE(IsPowerOfTwo<T>(power_of_2));
        if (i > 0 && (power_of_2 + this->one_val) < this->max_val && power_of_2 < this->max_val) {
            // Avoid overflow for +1
            // Ensure power_of_2+1 is not itself a power of 2 (unless power_of_2 was 0, which is not the case here)
            if (power_of_2 > 0 && (power_of_2 + this->one_val) != (power_of_2 << 1))
                EXPECT_FALSE(HasSingleBit<T>(power_of_2 + this->one_val));
        }
    }
    if (this->bits_in_T >= 2) {
        EXPECT_FALSE(HasSingleBit(static_cast<T>(3))); // 0b11
    }
    EXPECT_FALSE(HasSingleBit(this->max_val)); // Unless T is uint1_t
}


TYPED_TEST(BitOpsTest, BitFloorPreviousPowerOfTwo)
{
    using T = TypeParam;
    EXPECT_EQ(BitFloor(this->zero_val), this->zero_val);
    EXPECT_EQ(PreviousPowerOfTwo(this->zero_val), this->zero_val);

    EXPECT_EQ(BitFloor(this->one_val), this->one_val);
    if (this->bits_in_T >= 3) {
        // 0b111 = 7
        EXPECT_EQ(BitFloor(static_cast<T>(7)), static_cast<T>(4));
        EXPECT_EQ(BitFloor(static_cast<T>(8)), static_cast<T>(8));
        EXPECT_EQ(BitFloor(static_cast<T>(9)), static_cast<T>(8));
    }
    if (this->bits_in_T > 0) {
        EXPECT_EQ(BitFloor(this->max_val), static_cast<T>(1) << (this->bits_in_T - 1));
    }
}

TYPED_TEST(BitOpsTest, BitCeilNextPowerOfTwo)
{
    using T = TypeParam;
    // Note: Behavior of BitCeil for values that would overflow needs to match std::bit_ceil (return 0)
    // Our fallback tries to achieve this.

    EXPECT_EQ(BitCeil(this->zero_val), this->one_val);
    EXPECT_EQ(NextPowerOfTwo(this->zero_val), this->one_val);
    EXPECT_EQ(BitCeil(this->one_val), this->one_val);

    if (this->bits_in_T >= 4) {
        // Test with 8, 9, 7
        EXPECT_EQ(BitCeil(static_cast<T>(7)), static_cast<T>(8));
        EXPECT_EQ(BitCeil(static_cast<T>(8)), static_cast<T>(8));
        EXPECT_EQ(BitCeil(static_cast<T>(9)), static_cast<T>(16));
    }

    // Test overflow case: next power of two for (max/2 + 1) up to (max) should be 0 if it overflows
    // e.g., uint8_t: max = 255. max/2 + 1 = 128 + 1 = 129. BitCeil(129) -> 256 (overflows uint8_t, should be 0)
    // std::bit_ceil for uint8_t of 129 is 0.
    // std::bit_ceil for uint8_t of 128 is 128.
    // std::bit_ceil for uint8_t of 255 is 0.
    if (this->bits_in_T > 0) {
        T halfway = (this->max_val / 2) + 1; // e.g. 128 for uint8_t if max is 255
        if (IsPowerOfTwo(halfway)) {
            EXPECT_EQ(BitCeil(halfway), halfway);
        }

        if (halfway < this->max_val && !IsPowerOfTwo(halfway)) {
            // e.g. 129 for uint8
            if ((static_cast<T>(BitFloor(halfway)) << 1) == 0 && BitFloor(halfway) != 0) {
                // Check if next one overflows
                EXPECT_EQ(BitCeil(halfway), static_cast<T>(0));
            }
            else if ((static_cast<T>(BitFloor(halfway)) << 1) != 0) {
                EXPECT_EQ(BitCeil(halfway), static_cast<T>(BitFloor(halfway)) << 1);
            }
        }
    }
}


TYPED_TEST(BitOpsTest, FloorLog2)
{
    using T = TypeParam;
    EXPECT_EQ(FloorLog2(this->zero_val), -1);
    EXPECT_EQ(FloorLog2(this->one_val), 0);
    if (this->bits_in_T >= 2)
        EXPECT_EQ(FloorLog2(static_cast<T>(2)), 1);
    if (this->bits_in_T >= 2)
        EXPECT_EQ(FloorLog2(static_cast<T>(3)), 1);
    if (this->bits_in_T >= 3)
        EXPECT_EQ(FloorLog2(static_cast<T>(7)), 2);
    if (this->bits_in_T >= 4)
        EXPECT_EQ(FloorLog2(static_cast<T>(8)), 3);
    if (this->bits_in_T > 0) {
        EXPECT_EQ(FloorLog2(this->max_val), this->bits_in_T - 1);
    }
}

TYPED_TEST(BitOpsTest, BitWidth)
{
    using T = TypeParam;
    EXPECT_EQ(BitWidth(this->zero_val), 0);
    EXPECT_EQ(BitWidth(this->one_val), 1);
    if (this->bits_in_T >= 2)
        EXPECT_EQ(BitWidth(static_cast<T>(2)), 2); // 0b10
    if (this->bits_in_T >= 2)
        EXPECT_EQ(BitWidth(static_cast<T>(3)), 2); // 0b11
    if (this->bits_in_T >= 3)
        EXPECT_EQ(BitWidth(static_cast<T>(7)), 3); // 0b111
    if (this->bits_in_T >= 4)
        EXPECT_EQ(BitWidth(static_cast<T>(8)), 4); // 0b1000
    if (this->bits_in_T > 0) {
        EXPECT_EQ(BitWidth(this->max_val), this->bits_in_T);
    }
}

TYPED_TEST(BitOpsTest, ReverseBits)
{
    using T = TypeParam;
    EXPECT_EQ(ReverseBits(this->zero_val), this->zero_val);
    EXPECT_EQ(ReverseBits(this->max_val), this->max_val);

    EXPECT_EQ(ReverseBits(static_cast<T>(0x01)), static_cast<T>(1) << (this->bits_in_T - 1));

    if (this->bits_in_T == 8) {
        EXPECT_EQ(ReverseBits(static_cast<T>(0x80)), static_cast<T>(1));    // if T is uint8_t
        EXPECT_EQ(ReverseBits(static_cast<T>(0xA0)), static_cast<T>(0x05)); // 10100000 -> 00000101
        EXPECT_EQ(ReverseBits(static_cast<T>(0x12)), static_cast<T>(0x48)); // 00010010 -> 01001000
    }

    if (this->bits_in_T == 16) {
        EXPECT_EQ(ReverseBits(static_cast<T>(0x1234)), static_cast<T>(0x2C48)); // 00010010 00110100 -> 00101100 01001000
    }
}

TYPED_TEST(BitOpsTest, RotateLeft)
{
    using T = TypeParam;
    if (this->bits_in_T == 0)
        return; // Skip for empty type if somehow possible
    EXPECT_EQ(RotateLeft(this->one_val, 0), this->one_val);
    EXPECT_EQ(RotateLeft(this->one_val, 1), static_cast<T>(2));
    EXPECT_EQ(RotateLeft(this->one_val, this->bits_in_T), this->one_val);
    EXPECT_EQ(RotateLeft(this->one_val, this->bits_in_T + 1), static_cast<T>(2));
    EXPECT_EQ(RotateLeft(this->one_val, -1), static_cast<T>(1) << (this->bits_in_T -1)); // Rotate right by 1

    T val = static_cast<T>(0xC3); // 11000011
    if (sizeof(T) == 1) {
        EXPECT_EQ(RotateLeft(val, 2), static_cast<T>(0x0F));  // 00001111
        EXPECT_EQ(RotateLeft(val, -2), static_cast<T>(0xF0)); // 11110000
    }
}

TYPED_TEST(BitOpsTest, RotateRight)
{
    using T = TypeParam;
    if (this->bits_in_T == 0)
        return;
    EXPECT_EQ(RotateRight(this->one_val, 0), this->one_val);
    EXPECT_EQ(RotateRight(this->one_val, 1), static_cast<T>(1) << (this->bits_in_T -1));
    EXPECT_EQ(RotateRight(this->one_val, this->bits_in_T), this->one_val);
    EXPECT_EQ(RotateRight(this->one_val, this->bits_in_T + 1), static_cast<T>(1) << (this->bits_in_T -1));
    EXPECT_EQ(RotateRight(this->one_val, -1), static_cast<T>(2)); // Rotate left by 1

    if (this->bits_in_T >= 8) {
        T val = static_cast<T>(0xC3); // 11000011
        if (sizeof(T) == 1) {
            // uint8_t
            EXPECT_EQ(RotateRight(val, 2), static_cast<T>(0xF0));  // 11110000
            EXPECT_EQ(RotateRight(val, -2), static_cast<T>(0x0F)); // rotl(val, 2)
        }
    }
}


TYPED_TEST(BitOpsTest, BitManipulation)
{
    using T = TypeParam;
    if (this->bits_in_T < 2)
        return; // Need at least 2 bits for these specific pos values

    T val = this->zero_val;
    val   = SetBit(val, 1); // 0...010
    EXPECT_EQ(val, static_cast<T>(2));
    EXPECT_TRUE(CheckBit(val, 1));
    EXPECT_FALSE(CheckBit(val, 0));

    val = SetBit(val, 0); // 0...011
    EXPECT_EQ(val, static_cast<T>(3));
    EXPECT_TRUE(CheckBit(val, 0));

    val = ToggleBit(val, 1); // 0...001
    EXPECT_EQ(val, static_cast<T>(1));
    EXPECT_FALSE(CheckBit(val, 1));

    val = ClearBit(val, 0); // 0...000
    EXPECT_EQ(val, this->zero_val);
    EXPECT_FALSE(CheckBit(val, 0));

    // Test with a higher bit
    if (this->bits_in_T >= 5) {
        val = SetBit(this->zero_val, 4); // 0...10000
        EXPECT_EQ(val, static_cast<T>(16));
        EXPECT_TRUE(CheckBit(val, 4));
    }
}

// Add more tests for edge cases and different values as needed.
// Especially for BitCeil's overflow behavior.

TYPED_TEST(BitOpsTest, BitCeilEdgeCases)
{
    using T = TypeParam;
    // Test value that is max_val / 2 + 1 (potential overflow for next power of 2)
    if (this->bits_in_T > 1) {
        // Avoid division by zero or meaningless for 1-bit type
        T mid_plus_1 = (this->max_val / 2) + 1;
        T expected_ceil;

        if (IsPowerOfTwo(mid_plus_1)) {
            expected_ceil = mid_plus_1;
        }
        else {
            // The next power of two after BitFloor(mid_plus_1)
            T floor_val = BitFloor(mid_plus_1);
            if (floor_val > this->max_val / 2) {
                // This implies floor_val is the MSB itself
                // This means floor_val is 100...0. Next power of two would be 100...00 (double)
                // which overflows if floor_val is MSB (1 << (bits-1))
                // so BitCeil should be 0.
                expected_ceil = static_cast<T>(0);
            }
            else if (floor_val == 0 && mid_plus_1 != 0) {
                // mid_plus_1 was non-zero, floor is 0 (e.g. mid_plus_1 = 0, floor=0)
                expected_ceil = static_cast<T>(1); // BitCeil(0) is 1.
            }
            else {
                // Check if floor_val << 1 overflows T
                // If (floor_val > 0) and ( (floor_val << 1) / 2 != floor_val ), it means overflow.
                // Or simpler: if floor_val has MSB set, then <<1 overflows (becomes 0 for unsigned).
                if (floor_val > 0 && (floor_val & (static_cast<T>(1) << (this->bits_in_T - 1)))) {
                    expected_ceil = static_cast<T>(0); // Shift would overflow
                }
                else {
                    expected_ceil = (floor_val == 0) ? static_cast<T>(1) : (floor_val << 1);
                }
            }
        }
        // This logic for expected_ceil is complex to get right for all T and match std::bit_ceil.
        // The actual test is against our BitCeil implementation.
        // The provided fallback for BitCeil aims to match std::bit_ceil.
        // std::bit_ceil((max()/2)+1) for uint8_t (128+1=129) is 0.
        // std::bit_ceil((max()/2)) for uint8_t (127) is 128.
        // std::bit_ceil(128) is 128.
        if (this->bits_in_T == 8) {
            EXPECT_EQ(BitCeil(static_cast<T>(127)), static_cast<T>(128));
            EXPECT_EQ(BitCeil(static_cast<T>(128)), static_cast<T>(128));
            EXPECT_EQ(BitCeil(static_cast<T>(129)), static_cast<T>(0)); // Overflow
            EXPECT_EQ(BitCeil(static_cast<T>(255)), static_cast<T>(0)); // Overflow
        }
        if (this->bits_in_T == 16) {
            EXPECT_EQ(BitCeil(static_cast<T>(32767)), static_cast<T>(32768)); // (2^15 - 1) -> 2^15
            EXPECT_EQ(BitCeil(static_cast<T>(32768)), static_cast<T>(32768)); // 2^15
            EXPECT_EQ(BitCeil(static_cast<T>(32769)), static_cast<T>(0));     // 2^15 + 1 -> overflows to 0
            EXPECT_EQ(BitCeil(static_cast<T>(65535)), static_cast<T>(0));     // 2^16 - 1 -> overflows to 0
        }
    }
}

TYPED_TEST(BitOpsTest, RoundUp)
{
    using T = TypeParam;

    // Test with y=1 (should always return x)
    EXPECT_EQ(RoundUp(static_cast<T>(0), static_cast<T>(1)), static_cast<T>(1));
    EXPECT_EQ(RoundUp(static_cast<T>(5), static_cast<T>(1)), static_cast<T>(5));
    EXPECT_EQ(RoundUp(this->max_val, static_cast<T>(1)), this->max_val);

    // Test general cases
    EXPECT_EQ(RoundUp(static_cast<T>(10), static_cast<T>(3)), static_cast<T>(12));
    EXPECT_EQ(RoundUp(static_cast<T>(12), static_cast<T>(3)), static_cast<T>(12)); // Already multiple
    EXPECT_EQ(RoundUp(static_cast<T>(13), static_cast<T>(3)), static_cast<T>(15));
    EXPECT_EQ(RoundUp(static_cast<T>(0), static_cast<T>(5)), static_cast<T>(5));
    EXPECT_EQ(RoundUp(static_cast<T>(5), static_cast<T>(5)), static_cast<T>(5));
    EXPECT_EQ(RoundUp(static_cast<T>(6), static_cast<T>(5)), static_cast<T>(10));

    // Test near max value (potential overflow in intermediate sum)
    // We need y > 1 for the assert check to be robust in the implementation.
    T near_max = this->max_val - 2;
    T divisor  = static_cast<T>(3);
    // Expected result: find multiple of 3 >= max_val - 2
    T expected = ((near_max + divisor - 1) / divisor) * divisor; // Manual calc using same logic
    // Skip test if max_val itself is too small for near_max to be meaningful or divisor > near_max
    if (this->max_val >= 3 && divisor > 1) {
        EXPECT_EQ(RoundUp(near_max, divisor), expected);

        // Test case where x = max_val
        divisor = 10;
        if (this->max_val > divisor) {
            // Ensure divisor makes sense
            // Check if max_val is already a multiple
            if (this->max_val % divisor == 0) {
                EXPECT_EQ(RoundUp(this->max_val, divisor), this->max_val);
            }
        }
    }
}

TYPED_TEST(BitOpsTest, AlignUp)
{
    using T = TypeParam;

    // Test with alignment=1 (should always return value)
    EXPECT_EQ(AlignUp(static_cast<T>(0), static_cast<T>(1)), static_cast<T>(0));
    EXPECT_EQ(AlignUp(static_cast<T>(5), static_cast<T>(1)), static_cast<T>(5));
    EXPECT_EQ(AlignUp(this->max_val, static_cast<T>(1)), this->max_val);

    // Test general cases (alignment must be power of 2)
    EXPECT_EQ(AlignUp(static_cast<T>(10), static_cast<T>(4)), static_cast<T>(12)); // Align 10 up to multiple of 4
    EXPECT_EQ(AlignUp(static_cast<T>(12), static_cast<T>(4)), static_cast<T>(12)); // Already aligned
    EXPECT_EQ(AlignUp(static_cast<T>(13), static_cast<T>(4)), static_cast<T>(16));
    EXPECT_EQ(AlignUp(static_cast<T>(0), static_cast<T>(8)), static_cast<T>(0));
    EXPECT_EQ(AlignUp(static_cast<T>(8), static_cast<T>(8)), static_cast<T>(8));
    EXPECT_EQ(AlignUp(static_cast<T>(9), static_cast<T>(8)), static_cast<T>(16));

    // Test larger alignment
    if (this->bits_in_T >= 16) {
        // Ensure alignment value fits
        EXPECT_EQ(AlignUp(static_cast<T>(1000), static_cast<T>(256)), static_cast<T>(1024));
        EXPECT_EQ(AlignUp(static_cast<T>(1024), static_cast<T>(256)), static_cast<T>(1024));
        EXPECT_EQ(AlignUp(static_cast<T>(1025), static_cast<T>(256)), static_cast<T>(1280)); // 1024 + 256
    }

    // Test near max value
    T alignment = static_cast<T>(1) << (this->bits_in_T / 2); // A power of 2 alignment
    if (alignment > 1 && this->max_val > alignment) {
        // Ensure alignment is valid and test is meaningful
        T near_max = this->max_val - (alignment / 2); // A value likely not aligned

        // Manual calculation for expected result:
        // If near_max % alignment == 0, result is near_max.
        // Otherwise, result is (near_max / alignment + 1) * alignment.
        // Using the bitwise formula directly for manual check:
        T expected = (near_max + alignment - 1) & ~(alignment - 1);
        // Check if expected result wrapped around (overflowed) - it shouldn't if near_max < max_val - alignment + 1
        if (expected < near_max && near_max != 0) {
            // Overflow likely happened if result is smaller than input (and input wasn't 0)
            // This might indicate the expected result truly exceeds max_val.
            // The AlignUp formula might naturally handle some overflows if value+align-1 wraps correctly before the AND.
            // However, relying on this specific wrap behavior is fragile.
            // The assert in AlignUp aims to catch the intermediate overflow.
        }

        EXPECT_EQ(AlignUp(near_max, alignment), expected);

        // Test alignment of max_val itself
        T expected_max = (this->max_val + alignment - 1) & ~(alignment - 1);
        // If max_val is already aligned, expected_max should be max_val.
        // If not, the operation might result in 0 due to overflow/wrap-around.
        // The exact behavior depends on how (max_val + alignment - 1) wraps.
        // If max_val = 0xFF, align = 4. max+align-1 = FF+3 = 102 (wraps to 2). ~3 = FC. 2 & FC = 0.
        if ((this->max_val & (alignment - 1)) == 0) {
            // check if max_val is already aligned
            EXPECT_EQ(AlignUp(this->max_val, alignment), this->max_val);
        }
        else {
            // If not aligned, the aligned-up value would be > max_val.
            // The bitwise formula often results in 0 in this case due to wrap-around.
            EXPECT_EQ(AlignUp(this->max_val, alignment), static_cast<T>(0)); // Common result of formula on overflow
        }
    }
}