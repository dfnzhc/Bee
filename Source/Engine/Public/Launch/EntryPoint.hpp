/**
 * @File EntryPoint.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>
#include <Core/Portability.hpp>

#ifdef BEE_IN_WINDOWS
extern int LaunchWindowsStartup(HINSTANCE, HINSTANCE, PSTR, int);
extern void LaunchWindowsShutdown();

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nCmdShow)
{
    int Result = LaunchWindowsStartup(hInInstance, hPrevInstance, pCmdLine, nCmdShow);
    LaunchWindowsShutdown();
    return Result;
}

#else
#  error "不支持的平台"
#endif