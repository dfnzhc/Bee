/**
 * @File EntryPoint.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>
#include <Core/Portability.hpp>

#include "Launch/LaunchParam.hpp"

namespace bee {

#ifdef BEE_IN_WINDOWS
extern BeeLaunchParam LaunchParamSetup(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nCmdShow);
extern int LaunchStartup(BeeLaunchParam&& launchParam);
#endif

extern void LaunchShutdown();

} // namespace bee

#ifdef BEE_IN_WINDOWS

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nCmdShow)
{
    int Result = bee::LaunchStartup(bee::LaunchParamSetup(hInInstance, hPrevInstance, pCmdLine, nCmdShow));
    bee::LaunchShutdown();
    return Result;
}

#endif