/**
 * @File LaunchWindows.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#include "Utility/Launch/LaunchParam.hpp"
#include "Utility/Logger.hpp"
#include "Utility/Error.hpp"

namespace bee {
extern int GuardedMain(const BeeLaunchParam& launchParam);

BEE_API int LaunchStartup(const BeeLaunchParam& launchParam)
{
    LogInfo("Launch Startup.");

    // TODO：捕捉异常
    int ErrorLevel = 0;
    ErrorLevel     = CatchAndReportAllExceptions([&launchParam] { return GuardedMain(launchParam); });

    return ErrorLevel;
}

BEE_API void LaunchShutdown()
{
    LogInfo("Launch Shutdown.");
}

} // namespace bee