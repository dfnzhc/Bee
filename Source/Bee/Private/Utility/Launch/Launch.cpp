/**
 * @File Launch.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/15
 * @Brief This file is part of Bee.
 */

#include "Utility/Launch/LaunchEngineLoop.hpp"
#include "Utility/Launch/LaunchParam.hpp"
#include "Core/Globals.hpp"
#include "Core/Version.hpp"

#include "Utility/Logger.hpp"
#include "Utility/Error.hpp"

namespace bee {

// TODO：命令行支持
int GuardedMain(const BeeLaunchParam& launchParam)
{
    // clang-format off
    LogInfo(R"(
_____________________________________________________________________
,-----.                    ,------.               ,--.                
|  |) /_  ,---.  ,---.     |  .---',--,--,  ,---. `--',--,--,  ,---.  
|  .-.  \| .-. :| .-. :    |  `--, |      \| .-. |,--.|      \| .-. : 
|  '--' /\   --.\   --.    |  `---.|  ||  |' '-' '|  ||  ||  |\   --. 
`------'  `----' `----'    `------'`--''--'.`-  / `--'`--''--' `----' 
                                           `---'           Ver. {}.{}.{}
_____________________________________________________________________)", BEE_VERSION_MAJOR, BEE_VERSION_MINOR, BEE_VERSION_PATCH);
    // clang-format on

    struct EngineLoopCleanupGuard
    {
        ~EngineLoopCleanupGuard() { LaunchLoop::Instance().shutdown(); }
    } CleanupGuard;

    // TODO：设置错误类型，使用 std::expected？
    int ErrorLevel = LaunchLoop::Instance().preInit(launchParam);
    if (ErrorLevel != 0) {
        return ErrorLevel;
    }

    ErrorLevel = LaunchLoop::Instance().init();

    while (!Globals::IsEngineExitRequested()) {
        LaunchLoop::Instance().tick();
    }

    return 0;
}
} // namespace bee