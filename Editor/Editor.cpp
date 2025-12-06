/**
 * @File Editor.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/2
 * @Brief This file is part of Bee.
 */

#include "Editor.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

#include <thread>

#include <Bee/Bee.hpp>

using namespace bee;

Editor::Editor()
{
}

Editor::~Editor()
{
}

bool Editor::init(int argc, char* argv[])
{
    // Logger::Instance().addSink(std::make_unique<BaseSink>());
    _running = true;

    return true;
}

int Editor::run()
{
    while (_running)
    {
        // BEE_INFO("🐝");
        std::cout << "🐝" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return EXIT_SUCCESS;
}
