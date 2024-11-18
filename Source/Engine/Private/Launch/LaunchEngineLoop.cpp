/**
 * @File LaunchEngineLoop.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */
 
#include "Launch/LaunchEngineLoop.hpp"
#include <Utility/Logger.hpp>

using namespace bee;


int EngineLoop::PreInit()
{
    LogInfo("EngineLoop::PreInit");
    
    return 0;
}

int EngineLoop::Init()
{
    LogInfo("EngineLoop::Init");
    return 0;
}

void EngineLoop::Tick()
{
//    LogInfo("EngineLoop::Tick");
}

void EngineLoop::Exit()
{
    LogInfo("EngineLoop::Exit");
}