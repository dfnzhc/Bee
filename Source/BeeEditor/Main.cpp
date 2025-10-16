/**
 * @File Main.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/12
 * @Brief This file is part of Bee.
 */

#include <iostream>
#include <Core/Core.hpp>

#include "Editor.hpp"

using namespace Bee;

int main()
{
    Editor editor;
    editor.initialize();

    return editor.run();
}
