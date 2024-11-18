/**
 * @File Launch.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/15
 * @Brief This file is part of Bee.
 */

#include "Launch/LaunchEngineLoop.hpp"
#include <Core/Globals.hpp>

using namespace bee;

// TODO：命令行支持
int GuardedMain()
{
    struct EngineLoopCleanupGuard
    {
        ~EngineLoopCleanupGuard() { EngineLoop::Instance().Exit(); }
    } CleanupGuard;
    
    // TODO：设置错误类型，使用 std::expected？
    int ErrorLevel = EngineLoop::Instance().PreInit();
    if ( ErrorLevel != 0)
    {
        return ErrorLevel;
    }
    
    ErrorLevel = EngineLoop::Instance().Init();
    
    while (!Globals::IsEngineExitRequested())
    {
        EngineLoop::Instance().Tick();
    }

    return 0;
}