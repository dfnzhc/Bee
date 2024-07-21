/**
 * @File ConstantTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/6/10
 * @Brief 
 */

#include <gtest/gtest.h>

#include "Bee/Bee.hpp"

using namespace bee;

TEST(Constant, TestZero)
{
    static_assert(std::is_arithmetic<decltype(Zero<int>())>::value, "Zero must return an arithmetic type.");
    EXPECT_EQ(0, Zero<int>());
    EXPECT_FLOAT_EQ(0.0f, Zero<float>());
    EXPECT_DOUBLE_EQ(0.0, Zero<double>());
}

TEST(Constant, TestOne)
{
    static_assert(std::is_arithmetic<decltype(One<int>())>::value, "One must return an arithmetic type.");
    EXPECT_EQ(1, One<int>());
    EXPECT_FLOAT_EQ(1.0f, One<float>());
    EXPECT_DOUBLE_EQ(1.0, One<double>());
}

TEST(Constant, TestTwo)
{
    static_assert(std::is_arithmetic<decltype(Two<int>())>::value, "Two must return an arithmetic type.");
    EXPECT_EQ(2, Two<int>());
    EXPECT_FLOAT_EQ(2.0f, Two<float>());
    EXPECT_DOUBLE_EQ(2.0, Two<double>());
}

TEST(PowTest, PositiveExponent)
{
    double base  = 2.0;
    int exponent = 3;
    EXPECT_DOUBLE_EQ(std::pow(base, exponent), Pow<3>(base));
}

TEST(PowTest, ZeroExponent)
{
    double base = 5.0;
    EXPECT_DOUBLE_EQ(1.0, Pow<0>(base)); // Any number to the power of 0 is 1
}

TEST(PowTest, OneExponent)
{
    double base = 7.0;
    EXPECT_DOUBLE_EQ(base, Pow<1>(base)); // Any number to the power of 1 is itself
}

TEST(PowTest, NegativeExponent)
{
    double base  = 5.0;
    int exponent = -2;
    EXPECT_DOUBLE_EQ(1 / (base * base), Pow<-2>(base)); // Reciprocal of the square
}
