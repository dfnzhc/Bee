/**
 * @File CoreTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/13
 * @Brief This file is part of Bee.
 */
 
#include <gtest/gtest.h>
#include <Core.hpp>
#include <libassert/assert-gtest.hpp>

using namespace bee;

TEST(Core, Test)
{
    EXPECT_EQ(bee::Test2(1, 2), 3);

    BEE_ASSERT(1 + 1 == 2, "Oh.");
    // BEE_ASSERT(1 + 1 == 3, "WTF.");
}