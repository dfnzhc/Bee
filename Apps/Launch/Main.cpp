/**
 * @File Main.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/27
 * @Brief This file is part of Bee.
 */

#include <Core/EntryPoint.hpp>
#include <Bee.hpp>

using namespace bee;

bee::LaunchParam LaunchParamSetup(int argc, char** argv)
{
    LaunchParam param;
    param.appCreator = [] {
        return std::make_unique<Application>();
    };

    return param;
}