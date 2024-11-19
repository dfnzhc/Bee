/**
 * @File LaunchEngineLoop.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */
 
#include "Launch/LaunchEngineLoop.hpp"
#include <Utility/Logger.hpp>
#include <Base/Window.hpp>
#include <Core/Globals.hpp>

using namespace bee;

namespace  {

std::unique_ptr<Window> window = nullptr;

} // namespace 

// TODO: 提供接口能使客户程序进行自定义

int EngineLoop::PreInit()
{
    LogInfo("EngineLoop::PreInit");
    
    return 0;
}

int EngineLoop::Init()
{
    LogInfo("EngineLoop::Init");
    
    Window::Desc desc;
    window = std::make_unique<Window>(desc, nullptr);
    window->setPos(50, 100);
    
    return 0;
}

void EngineLoop::Tick()
{
    //    LogInfo("EngineLoop::Tick");
    
    window->pollForEvents();
    
    if (window->shouldClose())
        Globals::RequestEngineExit();
}

void EngineLoop::Shutdown()
{
    window->shutdown();
    
    LogInfo("EngineLoop::Shutdown");
}