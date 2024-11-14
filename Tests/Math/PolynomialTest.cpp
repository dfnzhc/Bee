/**
 * @File PolynomialTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Polynomial.hpp>
#include <libassert/assert-gtest.hpp>

using namespace bee;

TEST(EvaluatePolynomialTest, SimplePolynomial)
{
    double t        = 2.0;                                  // Point to evaluate the polynomial at
    double result   = EvaluatePolynomial(t, 3.0, 2.0, 1.0); // Represents 3 + 2t + t^2
    double expected = 3 + 2 * 2 + 2 * 2;                    // Manual calculation
    EXPECT_DOUBLE_EQ(expected, result);
}

TEST(EvaluatePolynomialTest, PolynomialWithZeroCoefficients)
{
    double t      = 5.0;
    double result = EvaluatePolynomial(t, 0.0, 0.0, 0.0); // Represents 0 + 0t + 0t^2
    EXPECT_DOUBLE_EQ(0.0, result);
}

TEST(EvaluatePolynomialTest, PolynomialWithConstantTerm)
{
    double t      = 0.0;                        // Any point will give the same result for a constant polynomial
    double result = EvaluatePolynomial(t, 7.0); // Represents 7
    EXPECT_DOUBLE_EQ(7.0, result);
}

TEST(DifferenceOfProductsTest, BasicOperation)
{
    float a        = 1.0f;
    float b        = 2.0f;
    float c        = 3.0f;
    float d        = 4.0f;
    float result   = DifferenceOfProducts(a, b, c, d);
    float expected = a * b - c * d;      // Expected result without FMA for comparison
    EXPECT_NEAR(expected, result, 1e-6); // Allow a small error margin
}

TEST(DifferenceOfProductsTest, ZeroValues)
{
    double a      = 0.0;
    double b      = 5.0;
    double c      = 0.0;
    double d      = 2.0;
    double result = DifferenceOfProducts(a, b, c, d);
    EXPECT_EQ(0.0, result); // Expect zero when any multiplicand is zero
}

TEST(DifferenceOfProductsTest, LargeValues)
{
    double a      = 1e308;
    double b      = 2e307;
    double c      = 3e306;
    double d      = 4e305;
    double result = DifferenceOfProducts(a, b, c, d);
    // The expected result might not be accurately representable due to overflow
    EXPECT_TRUE(std::isnan(result)); // Expect infinity or similar
}

TEST(SumOfProductsTest, BasicOperation)
{
    float a        = 1.0f;
    float b        = 2.0f;
    float c        = 3.0f;
    float d        = 4.0f;
    float result   = SumOfProducts(a, b, c, d);
    float expected = a * b + c * d;      // Expected result without FMA for comparison
    EXPECT_NEAR(expected, result, 1e-5); // Allow a small error margin
}

TEST(SumOfProductsTest, ZeroValues)
{
    double a      = 0.0;
    double b      = 5.0;
    double c      = 0.0;
    double d      = 2.0;
    double result = SumOfProducts(a, b, c, d);
    EXPECT_EQ(0.0, result); // Expect 'b' when 'a' and 'c' are zero
}

TEST(SumOfProductsTest, LargeValues)
{
    double a      = 1e308;
    double b      = 1e308;
    double c      = 1e308;
    double d      = 1e308;
    double result = SumOfProducts(a, b, c, d);
    // The expected result might not be accurately representable due to overflow
    EXPECT_TRUE(std::isnan(result)); // Expect infinity or similar
}

TEST(QuadraticTest, StandardCase)
{
    float a = 1.0f;
    float b = -3.0f;
    float c = 2.0f;
    float t0, t1;

    EXPECT_TRUE(Quadratic(a, b, c, &t0, &t1));
    EXPECT_NEAR(1.0f, t0, 1e-5);
    EXPECT_NEAR(2.0f, t1, 1e-5);
}

TEST(QuadraticTest, DoubleRootCase)
{
    double a = 1.0;
    double b = 2.0;
    double c = 1.0;
    double t0, t1;

    EXPECT_TRUE(Quadratic(a, b, c, &t0, &t1));
    EXPECT_DOUBLE_EQ(t0, t1);
    EXPECT_DOUBLE_EQ(-1, t0);
    EXPECT_DOUBLE_EQ(-1, t1);
}

TEST(QuadraticTest, ImaginaryRootsCase)
{
    double a = 1.0;
    double b = 0.0;
    double c = 1.0;
    double t0, t1;

    EXPECT_FALSE(Quadratic(a, b, c, &t0, &t1));
}