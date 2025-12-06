/**
 * @File TestCore.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/25
 * @Brief This file is part of Bee.
 */

#include "Check.hpp"

BEE_TEST_KERNAL(TestCoreKernal)
{
    int x = 10;
    int y = 20;

    EXPECT_EQ(x, 10);
    EXPECT_EQ(x, y);
    EXPECT_GT(y, x);

    ASSERT_EQ(x, 10);
    ASSERT_EQ(x, 999);

    printf("[FC] %d\n", testStatus->failureCount);
}

BEE_DEFINE_TEST(TestCore);
