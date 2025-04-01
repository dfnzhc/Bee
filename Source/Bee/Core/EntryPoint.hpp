/**
 * @File EntryPoint.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/1
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Launch.hpp"

extern bee::LaunchParam LaunchParamSetup(int argc, char** argv);

int main(int argc, char** argv)
{
    int Result = bee::Launch(LaunchParamSetup(argc, argv));
    bee::Shutdown();
    return Result;
}