/**
 * @File ErrorTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/31
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee.hpp>

using namespace bee;

TEST(ResultTest, UnexpectedNoDangling) {
    auto err = bee::Unexpected("Value {} invalid", 5);
    EXPECT_EQ(err.error(), "Value 5 invalid");
}

TEST(ExceptionTest, ThrowMacro) {
    EXPECT_THROW({
        try { BEE_THROW("Test error"); }
        catch (const bee::RuntimeError& e) {
            EXPECT_TRUE(String(e.what()).find("Test error") != String::npos);
            throw;
        }
    }, bee::RuntimeError);
}

TEST(AssertTest, AssertOpWithComma) {
    auto pair = std::make_pair(1, 2);
    EXPECT_THROW(BEE_ASSERT_EQ(pair, std::make_pair(2, 1)), bee::AssertionError);
}

TEST(GuardianTest, HandlesExceptions) {
    auto result = bee::Guardian([]() -> int {
        throw bee::RuntimeError("Guardian test");
        return 0;
    });
    
    EXPECT_EQ(result, EXIT_FAILURE);
}

TEST(GuardianTest, Terminate) {
    auto result0 = bee::Guardian([]() -> int {
        std::terminate();
        return 0;
    });

    auto result1 = bee::Guardian([]() -> int {
       std::abort();
       return 0;
   });
    
    EXPECT_EQ(result0, EXIT_FAILURE);
    EXPECT_EQ(result1, EXIT_FAILURE);
}