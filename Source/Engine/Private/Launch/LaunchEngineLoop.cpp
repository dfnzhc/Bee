/**
 * @File LaunchLoop.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#include "Launch/LaunchEngineLoop.hpp"
#include <Utility/Logger.hpp>
#include "Base/Globals.hpp"
#include "Base/Application.hpp"

namespace {

std::unique_ptr<bee::Application> app = nullptr;

} // namespace

// TODO: 提供接口能使客户程序进行自定义

namespace bee {

int LaunchLoop::preInit(const BeeLaunchParam& param)
{
    LogInfo("EngineLoop::preInit");
    app = param.createFunc();
    BEE_ASSERT(app, "未能创建应用程序实例");

    app->preInit();

    return 0;
}

int LaunchLoop::init()
{
    LogInfo("EngineLoop::init");

    app->init();

    return 0;
}

void LaunchLoop::tick()
{
    BEE_DEBUG_ASSERT(app);

    app->tick();
    if (app->shouldTerminate())
        Globals::RequestEngineExit();
}

void LaunchLoop::shutdown()
{
    if (app)
        app->shutdown();
    LogInfo("EngineLoop::shutdown");
}
} // namespace bee