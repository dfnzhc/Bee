/**
 * @File Engine.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */


#include "Core/Engine.hpp"

using namespace bee;

Engine::Engine()
{
    
}

Engine::~Engine()
{
    _window.reset();
}

int Engine::execute()
{
    if (!_initialize()) {
        LogError("Engine initialized failed.");
        return EXIT_FAILURE;
    }

    while (!_window->isRequestExit()) {
        _window->pollForEvents();
    }
    
    _shutdown();

    return EXIT_SUCCESS;
}

void Engine::onWindowSizeChanged(int width, int height)
{
    // 
}

bool Engine::_initialize()
{
    _window = std::make_unique<Window>(this);
    SimpleCheckAndReturn(_window);

    // TODO: Is an elegant way to handle exceptions?
    try {
        _window->initialize();
    } catch (...) {
        return false;
    }

    return true;
}

void Engine::_shutdown()
{
    _window->shutdown();
}