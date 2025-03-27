/**
 * @File Main.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/27
 * @Brief This file is part of Bee.
 */

#include "Bee.hpp"

using namespace bee;

int Run(int argc, char** argv)
{
    std::cout << "Hello Bee" << std::endl;
    // TODO: return App.Run(argc, argv);
    return 0;
}

int main(int argc, char** argv)
{
    return Guardian([&] { return Run(argc, argv); });
}