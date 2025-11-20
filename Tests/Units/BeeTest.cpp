/**
 * @File BeeTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/20
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <App/Application.hpp>

using namespace Bee;

TEST(BeeTest, TestDummy)
{
    EXPECT_EQ(Bee::Test(40, 2), 42);
}
