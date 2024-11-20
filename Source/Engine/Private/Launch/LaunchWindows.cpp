/**
 * @File LaunchWindows.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#include "Launch/LaunchEngineLoop.hpp"
#include "Launch/LaunchParam.hpp"

#include <Utility/Logger.hpp>

namespace bee {
extern int GuardedMain(BeeLaunchParam&& launchParam);

BEE_API int LaunchStartup(BeeLaunchParam&& launchParam)
{
    LogInfo("Launch Startup.");

    // TODO：捕捉异常
    int ErrorLevel = 0;
    ErrorLevel     = GuardedMain(std::forward<BeeLaunchParam>(launchParam));

    return ErrorLevel;
}

BEE_API void LaunchShutdown()
{
    LogInfo("Launch Shutdown.");
}

} // namespace bee