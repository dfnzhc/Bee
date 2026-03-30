#include <gtest/gtest.h>

#include "Math/Math.hpp"

TEST(MathComponentTests, ClampKeepsValueInRange)
{
    EXPECT_EQ(bee::Clamp(5, 0, 10), 5);
    EXPECT_EQ(bee::Clamp(-1, 0, 10), 0);
    EXPECT_EQ(bee::Clamp(42, 0, 10), 10);
}

TEST(MathComponentTests, LerpInterpolates)
{
    EXPECT_FLOAT_EQ(bee::Lerp(0.0f, 10.0f, 0.25f), 2.5f);
    EXPECT_DOUBLE_EQ(bee::Lerp(2.0, 6.0, 0.5), 4.0);
}

TEST(MathComponentTests, DetectsPowerOfTwo)
{
    EXPECT_TRUE(bee::IsPowerOfTwo(1));
    EXPECT_TRUE(bee::IsPowerOfTwo(64));
    EXPECT_FALSE(bee::IsPowerOfTwo(0));
    EXPECT_FALSE(bee::IsPowerOfTwo(70));
}
