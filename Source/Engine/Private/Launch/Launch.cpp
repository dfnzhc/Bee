/**
 * @File Launch.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/15
 * @Brief This file is part of Bee.
 */

#include "Launch/LaunchEngineLoop.hpp"
#include "Launch/LaunchParam.hpp"
#include "Base/Globals.hpp"
#include <Utility/Logger.hpp>
#include <Utility/Error.hpp>
#include <Core/Version.hpp>

namespace bee {

// TODO：命令行支持
int GuardedMain(const BeeLaunchParam& launchParam)
{
    LogInfo(R"(
,-----.                    ,------.               ,--.                
|  |) /_  ,---.  ,---.     |  .---',--,--,  ,---. `--',--,--,  ,---.  
|  .-.  \| .-. :| .-. :    |  `--, |      \| .-. |,--.|      \| .-. : 
|  '--' /\   --.\   --.    |  `---.|  ||  |' '-' '|  ||  ||  |\   --. 
`------'  `----' `----'    `------'`--''--'.`-  / `--'`--''--' `----' 
                                           `---'           Ver. {}.{}.{})", BEE_VERSION_MAJOR, BEE_VERSION_MINOR, BEE_VERSION_PATCH);
    
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