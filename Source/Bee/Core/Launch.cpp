/**
 * @File Launch.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/1
 * @Brief This file is part of Bee.
 */

#include "Core/Launch.hpp"

#include "Engine.hpp"
#include "Config.hpp"
#include "Base/Logger.hpp"

using namespace bee;

namespace {
// ==================
// Entry Point
// ==================
static int BeeMain(const LaunchParam& launchParam)
{
    LogInfo("Bee Engine | Ver. {}.{}.{}", BEE_VERSION_MAJOR, BEE_VERSION_MINOR, BEE_VERSION_PATCH);

    auto engine = launchParam.engineCreator();

    return engine->execute();
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