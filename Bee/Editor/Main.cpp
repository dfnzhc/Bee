/**
 * @File Main.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/20
 * @Brief This file is part of Bee.
 */

#include <iostream>
#include <Bee/Bee.hpp>

#include "Editor.hpp"

using namespace bee;

int main(int argc, char* argv[])
{
    Editor editor;

    if (!editor.init(argc, argv))
    {
        std::cerr << "Failed to initialize." << std::endl;
        return EXIT_FAILURE;
    }

    return editor.run();
}
