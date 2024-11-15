/**
 * @File LoggerTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */
 

#include <gtest/gtest.h>
#include <Bee.hpp>
#include <libassert/assert-gtest.hpp>

using namespace bee;

TEST(LoggerTest, BasicOutput)
{
    int a         = 1;
    float b       = 2.f;
    std::string c = "3.0";
    
    LogInfo("Hello from info");
    LogInfo("Hello from info: {} {} {}", a, b, c);

    LogWarn("Hello from warn");
    LogWarn("Hello from warn: {} {} {}", a, b, c);

    LogError("Hello from error");
    LogError("Hello from error: {} {} {}", a, b, c);

    LogError("Hello from fatal");
    LogFatal("Hello from fatal: {} {} {}", a, b, c);
}