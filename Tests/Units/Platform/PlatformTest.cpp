/**
 * @File PlatformTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/11
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee/Bee.hpp>

#include "Bee/Platform/Platform.hpp"

using namespace bee;

class PlatformTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        initialized = Platform::Initialize();
    }

    void TearDown() override
    {
        Platform::Shutdown();
    }

    bool initialized = false;
};

TEST_F(PlatformTest, Init)
{
    ASSERT_TRUE(initialized);
}