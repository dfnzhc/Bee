/**
 * @File Launch.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/1
 * @Brief This file is part of Bee.
 */

#include "Core/Launch.hpp"

#include "Config.hpp"
#include "Base/Logger.hpp"

using namespace bee;

namespace {
// ==================
// Entry Point
// ==================
static int BeeMain(const LaunchParam& launchParam)
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
        ~EngineLoopCleanupGuard()
        {
        }
    } CleanupGuard;

    return 0;
}
} // namespace 

namespace bee {

BEE_API int Launch(const LaunchParam& launchParam)
{
    Logger::Setup(true);
    LogInfo("Launching...");

    int ErrorCode = EXIT_SUCCESS;
    ErrorCode     = Guardian([&launchParam] { return BeeMain(launchParam); });

    return ErrorCode;
}

BEE_API void Shutdown()
{
    LogInfo("Shutdown.");
}

} // namespace bee