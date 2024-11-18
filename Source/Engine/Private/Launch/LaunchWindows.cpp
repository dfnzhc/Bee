/**
 * @File LaunchWindows.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#include "Launch/LaunchEngineLoop.hpp"
#include <Utility/Logger.hpp>

#include <Windows.h>

extern int GuardedMain();

using namespace bee;

BEE_API int LaunchWindowsStartup(HINSTANCE hInInstance, HINSTANCE hPrevInstance, char*, int nCmdShow)
{
    LogInfo("Windows Launch Startup.");

    int ErrorLevel = 0;

    ErrorLevel = GuardedMain();

    return ErrorLevel;
}

BEE_API void LaunchWindowsShutdown()
{
    LogInfo("Windows Launch Shutdown.");
}