/**
 * @File Engine.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */

#include "Core/Engine.hpp"
#include "Core/Window.hpp"

#include "Core/UI/Gui.hpp"
#include "Events/EventManager.hpp"

using namespace bee;

Engine::Engine()
{
}

Engine::~Engine()
{
    _shutdown();
}

int Engine::execute()
{
    if (!_initialize()) {
        LogError("Engine initialized failed.");
        return EXIT_FAILURE;
    }

    while (!_pWindow->isRequestExit()) {
        _pWindow->pollForEvents();

        _update();

        EventManager::Instance().Process();
    }

    _shutdown();

    return EXIT_SUCCESS;
}

void Engine::_onWindowSizeChanged(int width, int height)
{
    _pGui->onWindowResize(width, height);
}

bool Engine::_initialize()
{
    _initializeWindow();
    SimpleCheckAndReturn(_pWindow);

    _initializeGui();
    SimpleCheckAndReturn(_pGui);

    return true;
}

void Engine::_shutdown()
{
    _shutdownWindow();

    _shutdownGui();
}

void Engine::_initializeWindow()
{
    _pWindow = std::make_unique<Window>();

    if (_pWindow) {
        _pWindow->initialize();
    }
}

void Engine::_shutdownWindow()
{
    if (_pWindow) {
        _pWindow->shutdown();
        _pWindow.reset();
        _pWindow = nullptr;
    }
}

void Engine::_initializeGui()
{
    _pGui = std::make_unique<Gui>(_pWindow->handleSDL(), _pWindow->rendererSDL(), _pWindow->dpiScale());

    // register input events
}

void Engine::_shutdownGui()
{
    if (_pGui) {
        _pGui.reset();
        _pGui = nullptr;
    }
}

void Engine::_update()
{
}