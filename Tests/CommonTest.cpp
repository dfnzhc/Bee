/**
 * @File CommonTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/27
 * @Brief 
 */

#include <gtest/gtest.h>

#include "Bee/Detail/Check.hpp"
#include "Bee/Detail/Common.hpp"

using namespace bee;

TEST(Common, TestSwap)
{
    int a = 5, b = 10;
    Swap(a, b);
    EXPECT_EQ(a, 10);
    EXPECT_EQ(b, 5);
}

TEST(Common, TestMemcpy)
{
    const char src[] = "Hello, World!";
    std::vector<char> dst(sizeof(src) + 1); // +1 for null terminator

    // Test copying to a non-null terminated string
    EXPECT_EQ(Memcpy(dst.data(), src, sizeof(src)), dst.data());

    // Test the result
    for (size_t i = 0; i < sizeof(src); ++i) {
        EXPECT_EQ(dst[i], src[i]);
    }

    // Test copying with null terminator
    dst.back() = '\0';           // Reset null terminator if needed
    EXPECT_EQ(Memcpy(dst.data(), src, sizeof(src) + 1), dst.data());
    EXPECT_EQ(dst.back(), '\0'); // Check if null terminator is preserved
}

TEST(Common, TestMemcmp)
{
    const char buf1[] = "Test Buffer 1";
    const char buf2[] = "Test Buffer 2";
    const char buf3[] = "Test Buffer 1";

    // Test equal buffers
    EXPECT_EQ(Memcmp(buf1, buf3, sizeof(buf1)), 0);

    // Test different buffers
    EXPECT_NE(Memcmp(buf1, buf2, sizeof(buf1)), 0);

    // Test with different sizes
    EXPECT_NE(Memcmp(buf1, buf2, sizeof(buf1) - 1), 0);
}

TEST(Common, TestMemset)
{
    std::vector<char> buffer(10, '\0'); // Initialize with null characters

    // Set buffer to a specific value
    EXPECT_EQ(Memset(buffer.data(), 1, buffer.size()), buffer.data());
    for (char c : buffer) {
        EXPECT_EQ(c, 1);
    }

    // Reset buffer to null characters
    EXPECT_EQ(Memset(buffer.data(), 0, buffer.size()), buffer.data());
    for (char c : buffer) {
        EXPECT_EQ(c, '\0');
    }
}

TEST(Common, TestBitCast)
{
    // 测试整数类型之间的转换
    int intValue     = 42;
    float floatValue = BitCast<float>(intValue);
    EXPECT_EQ(sizeof(int), sizeof(float)); // 确保测试是有效的
    EXPECT_EQ(0, std::memcmp(&intValue, &floatValue, sizeof(int)));

    // 测试指针类型之间的转换
    char charValue    = 'A';
    char* charPointer = &charValue;
    int* intPointer   = BitCast<int*>(charPointer);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(charPointer), reinterpret_cast<uintptr_t>(intPointer));

    // 测试结构体之间的转换，假设两个结构体具有相同的内存布局
    struct A
    {
        char a;
        double b;
    };
    struct B
    {
        char c;
        double d;
    };
    A aStruct = {{'x'}, 3.14};
    B bStruct = BitCast<B>(aStruct);
    EXPECT_EQ(aStruct.a, bStruct.c);
    EXPECT_EQ(aStruct.b, bStruct.d);
}

TEST(Common, TestEqual)
{
    EXPECT_TRUE(Equal<i32>(42, 42));
    EXPECT_FALSE(Equal<i32>(42, -42));
}

TEST(Common, TestNotEqual)
{
    EXPECT_TRUE(NotEqual<f32>(42, -42));
    EXPECT_FALSE(NotEqual<f64>(42, 42));
}

TEST(Common, TestMin)
{
    EXPECT_EQ(Min<i32>(42, 27), 27);
    EXPECT_EQ(Min<i32>(42, 42, 27), 27);
    EXPECT_EQ(Min<i32>(42, 42, 27, 18), 18);
}

TEST(Common, TestMax)
{
    EXPECT_EQ(Max<u32>(42, 27), 42);
    EXPECT_EQ(Max<u32>(42, 42, 27), 42);
    EXPECT_EQ(Max<u32>(42, 42, 27, 18), 42);
}

TEST(Common, TestClamp)
{
    EXPECT_EQ(Clamp<u32>(10, 5, 20), 10);
    EXPECT_EQ(Clamp<u32>(3, 5, 20), 5);
    EXPECT_EQ(Clamp<u32>(25, 5, 20), 20);
}

TEST(Common, TestAbs)
{
    EXPECT_EQ(Abs<i32>(-42), 42);
    EXPECT_EQ(Abs<i32>(42), 42);
}

TEST(Common, TestSign)
{
    EXPECT_EQ(Sign<i32>(-42), -1);
    EXPECT_EQ(Sign<i32>(42), 1);
    EXPECT_EQ(Sign<i32>(0), 0);
}

TEST(FloorTest, TestFloorFloat)
{
    EXPECT_FLOAT_EQ(Floor(3.14f), 3.0f);
    EXPECT_FLOAT_EQ(Floor(-3.14f), -4.0f);
    EXPECT_FLOAT_EQ(Floor(0.0f), 0.0f);
}

TEST(FloorTest, TestFloorDouble)
{
    EXPECT_DOUBLE_EQ(Floor(3.14), 3.0);
    EXPECT_DOUBLE_EQ(Floor(-3.14), -4.0);
    EXPECT_DOUBLE_EQ(Floor(0.0), 0.0);
}

TEST(CeilTest, TestCeilFloat)
{
    EXPECT_FLOAT_EQ(Ceil(3.14f), 4.0f);
    EXPECT_FLOAT_EQ(Ceil(-3.14f), -3.0f);
    EXPECT_FLOAT_EQ(Ceil(0.0f), 0.0f);
}

TEST(CeilTest, TestCeilDouble)
{
    EXPECT_DOUBLE_EQ(Ceil(3.14), 4.0);
    EXPECT_DOUBLE_EQ(Ceil(-3.14), -3.0);
    EXPECT_DOUBLE_EQ(Ceil(0.0), 0.0);
}

TEST(TruncTest, TestTruncFloat)
{
    EXPECT_FLOAT_EQ(Trunc(3.14f), 3.0f);
    EXPECT_FLOAT_EQ(Trunc(-3.14f), -3.0f);
    EXPECT_FLOAT_EQ(Trunc(-0.999f), -0.0f); // 验证负数的舍入行为
    EXPECT_FLOAT_EQ(Trunc(0.0f), 0.0f);
}

TEST(TruncTest, TestTruncDouble)
{
    EXPECT_DOUBLE_EQ(Trunc(3.14), 3.0);
    EXPECT_DOUBLE_EQ(Trunc(-3.14), -3.0);
    EXPECT_DOUBLE_EQ(Trunc(-0.999), -0.0); // 验证负数的舍入行为
    EXPECT_DOUBLE_EQ(Trunc(0.0), 0.0);
}

TEST(RoundTest, TestRoundFloat)
{
    EXPECT_FLOAT_EQ(Round(3.14f), 3.0f);
    EXPECT_FLOAT_EQ(Round(-3.14f), -3.0f);
    EXPECT_FLOAT_EQ(Round(2.5f), 3.0f);   // 验证正数的四舍五入
    EXPECT_FLOAT_EQ(Round(3.5f), 4.0f);
    EXPECT_FLOAT_EQ(Round(-2.5f), -3.0f); // 验证负数的四舍五入
    EXPECT_FLOAT_EQ(Round(-3.5f), -4.0f);
    EXPECT_FLOAT_EQ(Round(0.0f), 0.0f);
}

TEST(RoundTest, TestRoundDouble)
{
    EXPECT_DOUBLE_EQ(Round(3.14), 3.0);
    EXPECT_DOUBLE_EQ(Round(-3.14), -3.0);
    EXPECT_DOUBLE_EQ(Round(2.5), 3.0);   // 验证正数的四舍五入
    EXPECT_DOUBLE_EQ(Round(3.5), 4.0);
    EXPECT_DOUBLE_EQ(Round(-2.5), -3.0); // 验证负数的四舍五入
    EXPECT_DOUBLE_EQ(Round(-3.5), -4.0);
    EXPECT_DOUBLE_EQ(Round(0.0), 0.0);
}

TEST(SqrtTest, TestSqrtPositive)
{
    EXPECT_FLOAT_EQ(Sqrt(9.0f), 3.0f);
    EXPECT_DOUBLE_EQ(Sqrt(16.0), 4.0);
    EXPECT_NEAR(Sqrt(2.0), 1.41421356, 1e-6); // 使用一个容忍度来比较浮点数
}

TEST(SqrtTest, TestSqrtZero)
{
    EXPECT_EQ(Sqrt(0.0f), 0.0f);
    EXPECT_EQ(Sqrt(0.0), 0.0);
}

TEST(ExpTest, TestExpPositive)
{
    EXPECT_NEAR(Exp(1.0f), 2.71828f, 1e-5f); // 测试 e^1
    EXPECT_NEAR(Exp(2.0), 7.38906, 1e-5);    // 测试 e^2
}

TEST(ExpTest, TestExpZero)
{
    EXPECT_EQ(Exp(0.0f), 1.0f); // 测试 e^0
    EXPECT_EQ(Exp(0.0), 1.0);   // 测试 e^0
}

TEST(ExpTest, TestExpNegative)
{
    EXPECT_NEAR(Exp(-1.0f), 0.36788f, 1e-5f); // 测试 e^-1
    EXPECT_NEAR(Exp(-2.0), 0.13534, 1e-5);    // 测试 e^-2
}

TEST(Exp2Test, TestExp2Positive)
{
    EXPECT_FLOAT_EQ(Exp2(1.0f), 2.0f); // 2^1
    EXPECT_NEAR(Exp2(2.0), 4.0, 1e-5); // 2^2
}

TEST(Exp2Test, TestExp2Zero)
{
    EXPECT_EQ(Exp2(0.0f), 1.0f); // 2^0
    EXPECT_EQ(Exp2(0.0), 1.0);   // 2^0
}

TEST(Exp2Test, TestExp2Negative)
{
    EXPECT_FLOAT_EQ(Exp2(-1.0f), 0.5f);  // 2^-1
    EXPECT_NEAR(Exp2(-2.0), 0.25, 1e-5); // 2^-2
}

// 测试 Exp2 函数的极端情况
TEST(Exp2Test, TestExp2Extremes)
{
    // 测试大值
    EXPECT_NEAR(Exp2(10.0), 1024.0, 1e-5);
    // 测试非常小的值
    EXPECT_NEAR(Exp2(-10.0), 0.0009765625, 1e-5);
}

TEST(LogTest, TestLogPositive)
{
    EXPECT_FLOAT_EQ(Log(1.0f), 0.0f);         // ln(1)
    EXPECT_NEAR(Log(2.71828f), 1.0f, 1e-5f);  // ln(e)
    EXPECT_NEAR(Log(10.0), 2.30258509, 1e-5); // ln(10)
}

TEST(LogTest, TestLogOne)
{
    EXPECT_EQ(Log(1.0), 0.0); // ln(1)
}

TEST(LogTest, TestLogE)
{
    EXPECT_NEAR(Log(std::exp(1.0)), 1.0, 1e-5); // ln(e^1)
}

TEST(Log2Test, TestLog2Positive)
{
    EXPECT_FLOAT_EQ(Log2(1.0f), 0.0f);    // log2(1)
    EXPECT_NEAR(Log2(2.0f), 1.0f, 1e-5f); // log2(2)
    EXPECT_NEAR(Log2(4.0), 2.0, 1e-5);    // log2(4)
    EXPECT_NEAR(Log2(8.0), 3.0, 1e-5);    // log2(8)
}

TEST(Log2Test, TestLog2One)
{
    EXPECT_EQ(Log2(1.0), 0.0); // log2(1)
}

TEST(Log2Test, TestLog2PowersOfTwo)
{
    for (int i = 0; i < 32 - 1; ++i) {
        EXPECT_NEAR(Log2(cast_to<f32>(1 << i)), cast_to<f32>(i), 1e-5f);
    }
}

TEST(Log10Test, TestLog10Positive)
{
    EXPECT_FLOAT_EQ(Log10(1.0f), 0.0f);     // log10(1)
    EXPECT_NEAR(Log10(10.0f), 1.0f, 1e-5f); // log10(10)
    EXPECT_NEAR(Log10(100.0), 2.0, 1e-5);   // log10(100)
}

TEST(Log10Test, TestLog10One)
{
    EXPECT_EQ(Log10(1.0), 0.0); // log10(1)
}

TEST(Log10Test, TestLog10PowersOfTen)
{
    for (int i = 1; i < 10; ++i) {
        EXPECT_NEAR(Log10(cast_to<f32>(std::pow(10, i))), cast_to<f32>(i), 1e-5f);
    }
}

TEST(SinTest, TestSinPi)
{
    EXPECT_NEAR(Sin(kPi), 0.0, 1e-5); // sin(π)
}

TEST(SinTest, TestSinZero)
{
    EXPECT_EQ(Sin(0.0), 0.0); // sin(0)
}

TEST(SinTest, TestSinPiOverTwo)
{
    EXPECT_NEAR(Sin(kHalfPi), 1.0, 1e-5); // sin(π/2)
}

TEST(SinTest, TestSinNegative)
{
    EXPECT_NEAR(Sin(-kHalfPi), -1.0, 1e-5); // sin(-π/2)
}

TEST(SinTest, TestSinMultiplePi)
{
    EXPECT_NEAR(Sin(kTwoPi), 0.0, 1e-5); // sin(2π)
}

TEST(SinhTest, TestSinhZero)
{
    EXPECT_EQ(Sinh(0.0), 0.0); // sinh(0)
}

TEST(SinhTest, TestSinhPositive)
{
    EXPECT_NEAR(Sinh(1.0), std::sinh(1.0), 1e-5); // sinh(1)
}

TEST(SinhTest, TestSinhNegative)
{
    EXPECT_NEAR(Sinh(-1.0), -std::sinh(1.0), 1e-5); // sinh(-1)
}

TEST(SinhTest, TestSinhPi)
{
    EXPECT_NEAR(Sinh(kPi), std::sinh(kPi), 1e-5); // sinh(π)
}

TEST(SinhTest, TestSinhInfinity)
{
    // Test behavior with infinity (expected to be infinity with the same sign)
    EXPECT_TRUE(std::isinf(Sinh(Infinity<f32>())));
    EXPECT_TRUE(std::isinf(Sinh(-Infinity<f32>())));
}

TEST(CosTest, TestCosZero)
{
    EXPECT_EQ(Cos(0.0), 1.0); // cos(0)
}

TEST(CosTest, TestCosPi)
{
    EXPECT_NEAR(Cos(kPi), -1.0, 1e-5); // cos(π)
}

TEST(CosTest, TestCosPiOverTwo)
{
    EXPECT_NEAR(Cos(kHalfPi), 0.0, 1e-5); // cos(π/2)
}

TEST(CosTest, TestCosNegative)
{
    EXPECT_NEAR(Cos(-kHalfPi), 0.0, 1e-5); // cos(-π/2)
}

TEST(CosTest, TestCosMultiplePi)
{
    EXPECT_NEAR(Cos(kTwoPi), 1.0, 1e-5); // cos(2π)
}

TEST(CoshTest, TestCoshZero)
{
    EXPECT_EQ(Cosh(0.0), 1.0); // cosh(0)
}

TEST(CoshTest, TestCoshPositive)
{
    EXPECT_NEAR(Cosh(1.0), std::cosh(1.0), 1e-5); // cosh(1)
}

TEST(CoshTest, TestCoshNegative)
{
    EXPECT_NEAR(Cosh(-1.0), std::cosh(-1.0), 1e-5); // cosh(-1)
}

TEST(CoshTest, TestCoshPi)
{
    EXPECT_NEAR(Cosh(kPi), std::cosh(kPi), 1e-5); // cosh(π)
}

TEST(CoshTest, TestCoshInfinity)
{
    // Test behavior with infinity (expected to be infinity)
    EXPECT_TRUE(std::isinf(Cosh(Infinity<f32>())));
    EXPECT_TRUE(std::isinf(Cosh(-Infinity<f32>())));
}
TEST(TanTest, TestTanZero)
{
    EXPECT_EQ(Tan(0.0), 0.0); // tan(0)
}

TEST(TanTest, TestTanPiOverFour)
{
    EXPECT_NEAR(Tan(kQuarterPi), 1.0, 1e-5); // tan(π/4)
}

TEST(TanTest, TestTanNegative)
{
    // Test the symmetry property: tan(-x) = -tan(x)
    const auto value = std::cos(-kQuarterPi) / std::sin(-kQuarterPi); // Expected value for tan(-π/4)
    EXPECT_NEAR(Tan(-kQuarterPi), value, 1e-5);
}

TEST(TanTest, TestTanInfinity)
{
    // tan(x) as x approaches an odd multiple of π/2 should tend to infinity or -infinity.
    EXPECT_TRUE(std::isnan(Tan(Infinity<f32>())));
    EXPECT_TRUE(std::isnan(Tan(-Infinity<f32>())));
}

TEST(TanhTest, TestTanhZero)
{
    EXPECT_EQ(Tanh(0.0), 0.0); // tanh(0)
}

TEST(TanhTest, TestTanhPositive)
{
    EXPECT_NEAR(Tanh(1.0), std::tanh(1.0), 1e-5); // tanh(1)
}

TEST(TanhTest, TestTanhNegative)
{
    EXPECT_NEAR(Tanh(-1.0), -std::tanh(1.0), 1e-5); // tanh(-1)
}

TEST(TanhTest, TestTanhPi)
{
    EXPECT_NEAR(Tanh(kPi), std::tanh(kPi), 1e-5); // tanh(π)
}

TEST(TanhTest, TestTanhInfinity)
{
    // Test behavior with infinity (expected to be -1 or 1)
    EXPECT_EQ(Tanh(Infinity<f32>()), 1.0);
    EXPECT_EQ(Tanh(-Infinity<f32>()), -1.0);
}

TEST(TanhTest, TestTanhNaN)
{
    // Test behavior with NaN (expected to be NaN)
    EXPECT_TRUE(std::isnan(Tanh(std::numeric_limits<f32>::quiet_NaN())));
}
TEST(aSinTest, TestaSinZero)
{
    EXPECT_NEAR(aSin(0.0), 0.0, 1e-5); // asin(0)
}

TEST(aSinTest, TestaSinOne)
{
    EXPECT_NEAR(aSin(1.0), kHalfPi, 1e-5); // asin(1) = π/2
}

TEST(aSinTest, TestaSinNegativeOne)
{
    EXPECT_NEAR(aSin(-1.0), -kHalfPi, 1e-5); // asin(-1) = -π/2
}

TEST(aSinTest, TestaSinPlusMinusInfinity)
{
    // asin(x) where x is infinity should be handled gracefully
    EXPECT_TRUE(std::isnan(aSin(Infinity<f32>())));
    EXPECT_TRUE(std::isnan(aSin(-Infinity<f32>())));
}

TEST(aSinTest, TestaSinOutOfRange)
{
    // asin(x) for x outside [-1, 1] should be handled gracefully
    EXPECT_TRUE(std::isnan(aSin(1.0001)));
    EXPECT_TRUE(std::isnan(aSin(-1.0001)));
}

TEST(aSinhTest, TestaSinhZero)
{
    EXPECT_NEAR(aSinh(0.0), 0.0, 1e-5); // asinh(0)
}

TEST(aSinhTest, TestaSinhPositive)
{
    EXPECT_NEAR(aSinh(1.0), std::asinh(1.0), 1e-5); // asinh(1)
}

TEST(aSinhTest, TestaSinhNegative)
{
    EXPECT_NEAR(aSinh(-1.0), -std::asinh(1.0), 1e-5); // asinh(-1)
}

TEST(aSinhTest, TestaSinhInfinity)
{
    EXPECT_EQ(aSinh(Infinity<f32>()), Infinity<f32>());
    EXPECT_EQ(aSinh(-Infinity<f32>()), -Infinity<f32>());
}

TEST(aSinhTest, TestaSinhNaN)
{
    // asinh(NaN) should return NaN
    EXPECT_TRUE(std::isnan(aSinh(std::numeric_limits<f32>::quiet_NaN())));
}

TEST(aCosTest, TestaCosOne)
{
    EXPECT_NEAR(aCos(1.0), 0.0, 1e-5); // acos(1) = 0
}

TEST(aCosTest, TestaCosNegativeOne)
{
    EXPECT_NEAR(aCos(-1.0), kPi, 1e-5); // acos(-1) = π
}

TEST(aCosTest, TestaCosZero)
{
    EXPECT_NEAR(aCos(0.0), kHalfPi, 1e-5); // acos(0) = π/2
}

TEST(aCosTest, TestaCosPlusMinusInfinity)
{
    // acos(x) where x is infinity should be handled gracefully
    EXPECT_TRUE(std::isnan(aCos(Infinity<f32>())));
    EXPECT_TRUE(std::isnan(aCos(-Infinity<f32>())));
}

TEST(aCosTest, TestaCosOutOfRange)
{
    // acos(x) for x outside [-1, 1] should be handled gracefully
    EXPECT_TRUE(std::isnan(aCos(1.0001)));
    EXPECT_TRUE(std::isnan(aCos(-1.0001)));
}

TEST(aCoshTest, TestaCoshOne)
{
    EXPECT_NEAR(aCosh(1.0), 0.0, 1e-5); // acosh(1) = 0
}

TEST(aCoshTest, TestaCoshPositive)
{
    EXPECT_NEAR(aCosh(2.0), std::acosh(2.0), 1e-5); // acosh(2)
}

TEST(aCoshTest, TestaCoshInfinity)
{
    // acosh(x) where x is infinity should return infinity
    EXPECT_TRUE(std::isinf(aCosh(Infinity<f32>())));
}

TEST(aCoshTest, TestaCoshNaN)
{
    // acosh(NaN) should return NaN
    EXPECT_TRUE(std::isnan(aCosh(std::numeric_limits<f32>::quiet_NaN())));
}

TEST(aCoshTest, TestaCoshOutOfRange)
{
    // acosh(x) for x < 1 should be handled gracefully
    EXPECT_TRUE(std::isnan(aCosh(0.5)));
}

TEST(aTanTest, TestaTanZero)
{
    EXPECT_NEAR(aTan(0.0), 0.0, 1e-5); // atan(0)
}

TEST(aTanTest, TestaTanOne)
{
    EXPECT_NEAR(aTan(1.0), kQuarterPi, 1e-5); // atan(1) = π/4
}

TEST(aTanTest, TestaTanNegative)
{
    // atan(-x) should be -atan(x)
    EXPECT_NEAR(aTan(-1.0), -kQuarterPi, 1e-5);
}

TEST(aTanTest, TestaTanInfinity)
{
    // atan(+infinity) should be π/2 and atan(-infinity) should be -π/2
    EXPECT_NEAR(aTan(Infinity<f32>()), kHalfPi, 1e-5);
    EXPECT_NEAR(aTan(-Infinity<f32>()), -kHalfPi, 1e-5);
}

TEST(aTanTest, TestaTanNaN)
{
    // atan(NaN) should return NaN
    EXPECT_TRUE(std::isnan(aTan(std::numeric_limits<f32>::quiet_NaN())));
}

TEST(aTanhTest, TestaTanhZero)
{
    EXPECT_NEAR(aTanh(0.0), 0.0, 1e-5); // atanh(0)
}

TEST(aTanhTest, TestaTanhPositive)
{
    EXPECT_NEAR(aTanh(0.5), std::atanh(0.5), 1e-5); // atanh(0.5)
}

TEST(aTanhTest, TestaTanhNegative)
{
    EXPECT_NEAR(aTanh(-0.5), -std::atanh(0.5), 1e-5); // atanh(-0.5)
}

TEST(aTanhTest, TestaTanhOne)
{
    // atanh(1) should be infinity
    EXPECT_EQ(aTanh(1.0), Infinity<f32>());
}

TEST(aTanhTest, TestaTanhNegativeOne)
{
    // atanh(-1) should be -infinity
    EXPECT_EQ(aTanh(-1.0), -Infinity<f32>());
}

TEST(aTanhTest, TestaTanhOutOfRange)
{
    // atanh(x) for x <= -1 or x >= 1 should be handled gracefully
    EXPECT_TRUE(std::isnan(aTanh(1.0001)));
    EXPECT_TRUE(std::isnan(aTanh(-1.0001)));
}

TEST(aTan2Test, TestaTan2PiOverFour)
{
    EXPECT_NEAR(aTan2(1.0, 1.0), kQuarterPi, 1e-5); // atan2(1, 1) = π/4
}

TEST(aTan2Test, TestaTan2ZeroOverZero)
{
    // atan2(0, 0) is implementation-defined, typically between -0 and 0
    EXPECT_TRUE(aTan2(0.0, 0.0) >= 0 && aTan2(0.0, 0.0) <= 0);
}

TEST(aTan2Test, TestaTan2ZeroOverPositive)
{
    EXPECT_NEAR(aTan2(0.0, 1.0), 0.0, 1e-5); // atan2(0, 1) = 0
}

TEST(aTan2Test, TestaTan2ZeroOverNegative)
{
    EXPECT_NEAR(aTan2(+0.0, -1.0), +kPi, 1e-5); // atan2(+0, -1) = +π
    EXPECT_NEAR(aTan2(-0.0, -1.0), -kPi, 1e-5); // atan2(-0, -1) = -π
}

TEST(aTan2Test, TestaTan2NegativeOverNegative)
{
    EXPECT_NEAR(aTan2(-1.0, -1.0), -3 * kQuarterPi, 1e-5); // atan2(-1, -1) = -3π/4
}

TEST(aTan2Test, TestaTan2InfinityOverInfinity)
{
    EXPECT_NEAR(aTan2(Infinity<f32>(), Infinity<f32>()), kQuarterPi, 1e-5);
}

TEST(aTan2Test, TestaTan2InfinityOverZero)
{
    EXPECT_NEAR(aTan2(Infinity<f32>(), 0.0f), kHalfPi, 1e-5);
}

TEST(aTan2Test, TestaTan2NegativeInfinityOverZero)
{
    EXPECT_NEAR(aTan2(-Infinity<f32>(), 0.0f), -kHalfPi, 1e-5);
}

TEST(ErfTest, TestErfZero)
{
    EXPECT_NEAR(Erf(0.0), 0.0, 1e-5); // erf(0)
}

TEST(ErfTest, TestErfPositive)
{
    EXPECT_NEAR(Erf(1.0), std::erf(1.0), 1e-5); // erf(1)
}

TEST(ErfTest, TestErfNegative)
{
    EXPECT_NEAR(Erf(-1.0), -std::erf(1.0), 1e-5); // erf(-1) = -erf(1)
}

TEST(ErfTest, TestErfInfinity)
{
    // erf(+infinity) should be 1 and erf(-infinity) should be -1
    EXPECT_EQ(Erf(Infinity<f32>()), 1.0);
    EXPECT_EQ(Erf(-Infinity<f32>()), -1.0);
}

TEST(ErfTest, TestErfNaN)
{
    // erf(NaN) should return NaN
    EXPECT_TRUE(std::isnan(Erf(std::numeric_limits<f32>::quiet_NaN())));
}

TEST(ErfInvTest, TestErfInvAtZero)
{
    EXPECT_NEAR(ErfInv(0.0), 0.0, 1e-5); // erfinv(0) should be 0
}

//TEST(ErfInvTest, TestErfInvAtOne)
//{
//    EXPECT_NEAR(ErfInv(1.0), Infinity<f32>(), 1e-5); // erfinv(1) should be infinity
//}
//
//TEST(ErfInvTest, TestErfInvAtNegativeOne)
//{
//    EXPECT_NEAR(ErfInv(-1.0), -Infinity<f32>(), 1e-5); // erfinv(-1) should be -infinity
//}

TEST(ErfInvTest, TestErfInvAtHalf)
{
    // erfinv(0.5) should be approximately 0.4769362762044
    EXPECT_NEAR(ErfInv(0.5), 0.4769362762044, 1e-5);
}

//TEST(ErfInvTest, TestErfInvOutOfRange)
//{
//    // erfinv(x) for x < -1 or x > 1 should be handled gracefully
//    // This behavior depends on the implementation of internal::ErfInv
//    EXPECT_TRUE(std::isnan(ErfInv(1.0001)));
//    EXPECT_TRUE(std::isnan(ErfInv(-1.0001)));
//}

TEST(CopySignTest, TestCopySignPositive)
{
    EXPECT_EQ(CopySign(3.0, 2.0), 3.0);   // Same sign
    EXPECT_EQ(CopySign(3.0, -2.0), -3.0); // Different sign
}

TEST(CopySignTest, TestCopySignZero)
{
    EXPECT_EQ(CopySign(0.0, 1.0), 0.0);   // Zero with positive sign
    EXPECT_EQ(CopySign(0.0, -1.0), -0.0); // Zero with negative sign
}

TEST(CopySignTest, TestCopySignInfinity)
{
    EXPECT_EQ(CopySign(Infinity<f32>(), -1.0f), -Infinity<f32>()); // Positive infinity to negative
    EXPECT_EQ(CopySign(-Infinity<f32>(), 1.0f), Infinity<f32>());  // Negative infinity to positive
}

TEST(CopySignTest, TestCopySignNaN)
{
    // NaN is a special case, the sign is typically ignored
    EXPECT_TRUE(std::isnan(CopySign(std::numeric_limits<f32>::quiet_NaN(), 1.0f)));
    EXPECT_TRUE(std::isnan(CopySign(std::numeric_limits<f32>::quiet_NaN(), -1.0f)));
}
TEST(fModTest, TestfModPositive)
{
    EXPECT_NEAR(fMod(5.0, 2.0), 1.0, 1e-5); // Positive result
}

TEST(fModTest, TestfModNegative)
{
    EXPECT_NEAR(fMod(-5.0, 2.0), -1.0, 1e-5); // Negative result
}

TEST(fModTest, TestfModZero)
{
    EXPECT_NEAR(fMod(0.0, 2.0), 0.0, 1e-5); // Zero divided by any non-zero number
}

TEST(fModTest, TestfModByZero)
{
    // fMod(x, 0) is undefined, but we expect it to be handled gracefully.
    // The behavior may vary by implementation, but typically it should raise an error.
    // This test checks for NaN as a result, but this behavior is not guaranteed.
    EXPECT_TRUE(std::isnan(fMod(5.0, 0.0)));
}

TEST(ModfTest, TestModfPositive)
{
    f64 ipart;
    f64 fpart = Modf(3.14159, &ipart);
    // Expect the integer part to be 3 and the fractional part to be ~0.14159
    EXPECT_EQ(ipart, 3);
    EXPECT_NEAR(fpart, 0.14159, 1e-5);
}

TEST(ModfTest, TestModfNegative)
{
    f64 ipart;
    f64 fpart = Modf(-3.14159, &ipart);
    // Expect the integer part to be -3 and the fractional part to be ~-0.14159
    EXPECT_EQ(ipart, -3);
    EXPECT_NEAR(fpart, -0.14159, 1e-5);
}

TEST(ModfTest, TestModfZero)
{
    f64 ipart;
    f64 fpart = Modf(0.0, &ipart);
    // Zero should have an integer part of 0 and a fractional part of 0
    EXPECT_EQ(ipart, 0);
    EXPECT_EQ(fpart, 0);
}

TEST(ModfTest, TestModfInfinity)
{
    f64 ipart;
    f64 fpart = Modf(Infinity<f64>(), &ipart);
    // Infinity should result in an integer part of infinity and a fractional part of 0
    EXPECT_TRUE(std::isinf(ipart));
    EXPECT_EQ(fpart, 0);
}

TEST(RadiansTest, TestRadiansZero)
{
    EXPECT_EQ(Radians(0.0), 0.0); // 0 degrees is 0 radians
}

TEST(RadiansTest, TestRadiansPi)
{
    EXPECT_NEAR(Radians(180.0), kPi, 1e-5); // 180 degrees is π radians
}

TEST(RadiansTest, TestRadiansNegative)
{
    EXPECT_NEAR(Radians(-180.0), -kPi, 1e-5); // -180 degrees is -π radians
}

TEST(RadiansTest, TestRadiansThreeSixty)
{
    EXPECT_NEAR(Radians(360.0), kTwoPi, 1e-5); // 360 degrees is 2π radians
}

TEST(DegreesTest, TestDegreesZero)
{
    EXPECT_EQ(Degrees(0.0), 0.0); // 0 radians is 0 degrees
}

TEST(DegreesTest, TestDegreesPi)
{
    EXPECT_NEAR(Degrees(kPi), 180.0, 1e-5); // π radians is 180 degrees
}

TEST(DegreesTest, TestDegreesNegative)
{
    EXPECT_NEAR(Degrees(-kPi), -180.0, 1e-5); // -π radians is -180 degrees
}

TEST(DegreesTest, TestDegreesTwoPi)
{
    EXPECT_NEAR(Degrees(kTwoPi), 360.0, 1e-5); // 2π radians is 360 degrees
}

TEST(FracTest, TestFracPositive)
{
    EXPECT_DOUBLE_EQ(Frac(3.14159), 0.14159); // Fractional part of a positive number
}

TEST(FracTest, TestFracNegative)
{
    EXPECT_DOUBLE_EQ(Frac(-3.14159), -0.14159); // Fractional part of a negative number
}

TEST(FracTest, TestFracZero)
{
    EXPECT_DOUBLE_EQ(Frac(0.0), 0.0); // Fractional part of zero is zero
}

TEST(FracTest, TestFracInteger)
{
    EXPECT_DOUBLE_EQ(Frac(-2.0), -0.0); // Fractional part of an integer is zero (negative zero in this case)
    EXPECT_DOUBLE_EQ(Frac(5.0), 0.0);   // Fractional part of an integer is zero
}

TEST(FracTest, TestFracInfinity)
{
    // The behavior of Frac with infinity is not well-defined, but typically it should be NaN or infinity.
    // This test checks for NaN as a result, but this behavior is not guaranteed.
    EXPECT_TRUE(std::isnan(Frac(Infinity<f32>())));
}

TEST(RcpTest, TestRcpPositive)
{
    EXPECT_NEAR(Rcp(2.0), 0.5, 1e-5);
}

TEST(RcpTest, TestRcpNegative)
{
    EXPECT_NEAR(Rcp(-2.0), -0.5, 1e-5);
}

TEST(RcpTest, TestRcpOne)
{
    EXPECT_EQ(Rcp(1.0), 1.0);
}

TEST(SaturateTest, TestSaturateWithinRange)
{
    EXPECT_EQ(Saturate(0.5f), 0.5f);
}

TEST(SaturateTest, TestSaturateBelowRange)
{
    EXPECT_EQ(Saturate(-0.5f), 0.0f);
}

TEST(SaturateTest, TestSaturateAboveRange)
{
    EXPECT_EQ(Saturate(1.5f), 1.0f);
}

TEST(SaturateTest, TestSaturateZero)
{
    EXPECT_EQ(Saturate(0.0f), 0.0f);
}

TEST(SaturateTest, TestSaturateOne)
{
    EXPECT_EQ(Saturate(1.0f), 1.0f);
}

TEST(StepTest, TestStepAbove)
{
    EXPECT_EQ(Step(0.5f, 1.0f), 1.0f); // x is above y
}

TEST(StepTest, TestStepBelow)
{
    EXPECT_EQ(Step(1.0f, 0.5f), 0.0f); // x is below y
}

TEST(StepTest, TestStepEqual)
{
    EXPECT_EQ(Step(1.0f, 1.0f), 1.0f); // x is equal to y
}

TEST(StepTest, TestStepZero)
{
    EXPECT_EQ(Step(0.0f, 0.0f), 1.0f); // x and y are both zero
}

TEST(StepTest, TestStepNegative)
{
    EXPECT_EQ(Step(-2.0f, -1.0f), 1.0f); // x is greater than y, both negative
    EXPECT_EQ(Step(-1.0f, -2.0f), 0.0f); // x is less than y, both negative
}

TEST(LerpTest, TestLerpZero)
{
    EXPECT_EQ(Lerp(0.0f, 1.0f, 0.0f), 0.0f); // s is 0, result should be x
}

TEST(LerpTest, TestLerpOne)
{
    EXPECT_EQ(Lerp(0.0f, 1.0f, 1.0f), 1.0f); // s is 1, result should be y
}

TEST(LerpTest, TestLerpMidpoint)
{
    EXPECT_EQ(Lerp(0.0f, 1.0f, 0.5f), 0.5f); // s is 0.5, result should be the midpoint between x and y
}

TEST(LerpTest, TestLerpNegative)
{
    EXPECT_EQ(Lerp(1.0f, -1.0f, 0.5f), 0.0f); // s is 0.5, result should be the midpoint between x and y
}

TEST(LerpTest, TestLerpWithNonLinear)
{
    EXPECT_NEAR(Lerp(2.0f, 5.0f, 0.25f), 2.75f, 1e-5); // s is 0.25, result should be 1/4 of the way between 2 and 5
}

TEST(BiLerpTest, TestBiLerpCorners)
{
    EXPECT_EQ(BiLerp(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f), 0.0f); // At v00
    EXPECT_EQ(BiLerp(0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f), 1.0f); // At v01
    EXPECT_EQ(BiLerp(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f), 1.0f); // At v10
    EXPECT_EQ(BiLerp(0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f), 0.0f); // At v11
}

TEST(BiLerpTest, TestBiLerpCenter)
{
    EXPECT_EQ(BiLerp(0.0f, 1.0f, 1.0f, 0.0f, 0.5f, 0.5f), 0.5f); // At the center of the grid
}

TEST(BiLerpTest, TestBiLerpDiagonal)
{
    EXPECT_NEAR(BiLerp(0.0f, 2.0f, 2.0f, 0.0f, 0.5f, 0.5f), 1.0f, 1e-5); // On the diagonal from v00 to v11
}

TEST(SmoothStepTest, TestSmoothStepWithinRange)
{
    EXPECT_NEAR(SmoothStep(0.5f, 0.0f, 1.0f), 0.5f, 1e-5);
}

TEST(SmoothStepTest, TestSmoothStepAtEdge0)
{
    EXPECT_NEAR(SmoothStep(0.0f, 0.0f, 1.0f), 0.0f, 1e-5);
}

TEST(SmoothStepTest, TestSmoothStepAtEdge1)
{
    EXPECT_NEAR(SmoothStep(1.0f, 0.0f, 1.0f), 1.0f, 1e-5);
}

TEST(SmoothStepTest, TestSmoothStepBelowRange)
{
    EXPECT_NEAR(SmoothStep(-0.5f, 0.0f, 1.0f), 0.0f, 1e-5);
}

TEST(SmoothStepTest, TestSmoothStepAboveRange)
{
    EXPECT_NEAR(SmoothStep(1.5f, 0.0f, 1.0f), 1.0f, 1e-5);
}

TEST(LengthTest, TestLengthPositive)
{
    EXPECT_EQ(Length(5.0f), 5.0f); // Positive value
}

TEST(LengthTest, TestLengthNegative)
{
    EXPECT_EQ(Length(-5.0f), 5.0f); // Negative value
}

TEST(LengthTest, TestLengthZero)
{
    EXPECT_EQ(Length(0.0f), 0.0f); // Zero value
}

TEST(DistanceTest, TestDistanceSamePoints)
{
    EXPECT_EQ(Distance(0.0f, 0.0f), 0.0f); // Distance between same points
}

TEST(DistanceTest, TestDistancePositive)
{
    EXPECT_EQ(Distance(0.0f, 5.0f), 5.0f); // Positive distance
}

TEST(DistanceTest, TestDistanceNegative)
{
    EXPECT_EQ(Distance(5.0f, 0.0f), 5.0f); // Distance with negative point difference
}

TEST(DistanceTest, TestDistanceZero)
{
    EXPECT_EQ(Distance(-3.0f, -3.0f), 0.0f); // Distance with zero value
}

TEST(DotTest, TestDotProductPositive)
{
    EXPECT_EQ(Dot(3.0f, 4.0f), 12.0f); // Positive numbers
}

TEST(DotTest, TestDotProductNegative)
{
    EXPECT_EQ(Dot(-3.0f, 4.0f), -12.0f); // One negative number
    EXPECT_EQ(Dot(-3.0f, -4.0f), 12.0f); // Both negative numbers
}

TEST(DotTest, TestDotProductZero)
{
    EXPECT_EQ(Dot(0.0f, 4.0f), 0.0f); // One zero
    EXPECT_EQ(Dot(0.0f, 0.0f), 0.0f); // Both zero
}

TEST(FaceForwardTest, TestAcuteAngle)
{
    EXPECT_EQ(FaceForward(1.0f, -1.0f, 1.0f), 1.0f);
}

TEST(FaceForwardTest, TestObtuseAngle)
{
    EXPECT_EQ(FaceForward(1.0f, -1.0f, 1.0f), 1.0f);
}

TEST(FaceForwardTest, TestZeroAngle)
{
    EXPECT_EQ(FaceForward(1.0f, 0.0f, 1.0f), -1.0f); // Zero angle (I is orthogonal to refN)
}

TEST(ApproxTest, TestApproxEqual)
{
    EXPECT_TRUE(Approx(0.0f, 0.0f));
    EXPECT_TRUE(Approx(1.0f + kShadowEpsilon, 1.0f));
}

TEST(ApproxTest, TestApproxNotEqual)
{
    EXPECT_FALSE(Approx(1.0f, 1.0f + 2 * kShadowEpsilon));
}

TEST(ApproxTest, TestApproxWithEpsilon)
{
    float epsilon = 0.1f; // Larger epsilon for this test
    EXPECT_TRUE(Approx(0.5f, 0.59f, epsilon));
}

TEST(SafeSqrtTest, TestSafeSqrtPositive)
{
    EXPECT_NEAR(SafeSqrt(9.0), 3.0, 1e-5);
}

TEST(SafeSqrtTest, TestSafeSqrtZero)
{
    EXPECT_EQ(SafeSqrt(0.0), 0.0);
}

TEST(SafeSqrtTest, TestSafeSqrtNegative)
{
    EXPECT_NEAR(SafeSqrt(-1e-4), 0.0, 1e-5); // Should treat as zero and return zero
}

TEST(SqrTest, TestSqrPositive)
{
    EXPECT_EQ(Sqr(3), 9);                // Positive integer
    EXPECT_NEAR(Sqr(2.5f), 6.25f, 1e-5); // Positive float
}

TEST(SqrTest, TestSqrNegative)
{
    EXPECT_EQ(Sqr(-3), 9);                // Negative integer
    EXPECT_NEAR(Sqr(-2.5f), 6.25f, 1e-5); // Negative float
}

TEST(SqrTest, TestSqrZero)
{
    EXPECT_EQ(Sqr(0), 0); // Zero
}

TEST(SinXOverXTest, NearZero)
{
    EXPECT_NEAR(1.0, SinXOverX(1e-5), 1e-4); // Expect the result to be close to 1
}

// Test with a positive value of x
TEST(SinXOverXTest, PositiveValue)
{
    EXPECT_NEAR(std::sin(0.5) / 0.5, SinXOverX(0.5), 1e-5); // Expect the sine value over the number itself
}

// Test with a negative value of x
TEST(SinXOverXTest, NegativeValue)
{
    EXPECT_NEAR(std::sin(-0.5) / -0.5, SinXOverX(-0.5), 1e-5); // Expect the sine value over the number itself
}

TEST(SafeASinTest, ValidInput)
{
    EXPECT_NEAR(std::asin(0.5), SafeASin(0.5), 1e-6);
}

// Test the SafeASin function with a value slightly outside the valid range
TEST(SafeASinTest, BoundaryInput)
{
    EXPECT_NEAR(std::asin(1.000), SafeASin(1.0001), 1e-6);   // Should clamp to 1 and return asin(1)
    EXPECT_NEAR(std::asin(-1.000), SafeASin(-1.0001), 1e-6); // Should clamp to -1 and return asin(-1)
}

TEST(SafeACosTest, WithinRange)
{
    EXPECT_NEAR(std::acos(0.5), SafeACos(0.5), 1e-6);
}

TEST(SafeACosTest, BoundaryValue)
{
    EXPECT_NEAR(std::acos(-1), SafeACos(-1.0001), 1e-6); // Should clamp to -1 and return acos(-1)
    EXPECT_NEAR(std::acos(1), SafeACos(1.0001), 1e-6);   // Should clamp to 1 and return acos(1)
}

TEST(GaussTest, MeanValue)
{
    // Evaluate the Gaussian function at the mean value, expected to be the peak of the distribution
    double mean   = 0.0;
    double stddev = 1.0;
    EXPECT_NEAR(1.0 / (stddev * std::sqrt(kTwoPi)), Gauss(0.0, mean, stddev), 1e-6);
}

TEST(GaussTest, OneStdDevFromMean)
{
    // Evaluate the Gaussian function one standard deviation away from the mean
    double mean     = 0.0;
    double stddev   = 1.0;
    double x        = mean + stddev;
    double expected = std::exp(-0.5) / (stddev * std::sqrt(kTwoPi));
    EXPECT_NEAR(expected, Gauss(x, mean, stddev), 1e-6);
}

TEST(GaussTest, NegativeValue)
{
    // Evaluate the Gaussian function for a negative value of x
    double mean   = 0.0;
    double stddev = 1.0;
    double x      = -1.0;
    EXPECT_NEAR(std::exp(-(x * x) / 2.0) / (stddev * std::sqrt(kTwoPi)), Gauss(x, mean, stddev), 1e-6);
}