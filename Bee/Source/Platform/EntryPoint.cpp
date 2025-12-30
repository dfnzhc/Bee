/**
 * @File EntryPoint.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/30
 * @Brief This file is part of Bee.
 */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Bee/Platform/IApplication.hpp"

using namespace bee;

namespace
{
std::unique_ptr<IApplication> App = nullptr;
} // namespace 

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    App = CreateApplication();
    if (App == nullptr)
        return SDL_APP_FAILURE;

    if (!App->initialize(argc, argv))
        return SDL_APP_FAILURE;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    if (App)
    {
        if (App->shouldQuit())
            return SDL_APP_SUCCESS;

        App->runFrame();
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (App)
    {
        // TODO: 处理事件
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    if (App)
    {
        App->shutdown();
    }
}
