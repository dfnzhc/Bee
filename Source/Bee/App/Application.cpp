/**
 * @File Application.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/20
 * @Brief This file is part of Bee.
 */

#include "Application.hpp"
#include <cstdio>
#include <print>

using namespace Bee;

void Bee::Hello()
{
    std::println("Hello form 🐝");
}

int Bee::Test(int a, int b)
{
    return a + b;
}
