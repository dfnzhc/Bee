/**
 * @File Platform.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/11
 * @Brief This file is part of Bee.
 */

#include "Platform.hpp"
#include "Utility/LogSink.hpp"

#include <SDL3/SDL.h>

using namespace bee;

bool Platform::Initialize()
{
    // TODO: 日志
    Logger::Instance().addSink(std::make_unique<LogSink>());
    BEE_INFO("正在初始化平台设施...");

    u32 sdlFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    if (!SDL_Init(sdlFlags))
    {
        BEE_ERROR("SDL3 初始化失败: {}.", SDL_GetError());
        return false;
    }

    return true;
}

void Platform::Shutdown()
{
    SDL_Quit();
    Logger::Instance().clearSinks();
}
