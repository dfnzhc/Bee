/**
 * @File Engine.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */

#include "Engine/Engine.hpp"

#include <Config.hpp>

#include "Core/Utility/Property.hpp"
#include "Core/Logger.hpp"
#include "Core/Error.hpp"

#include "Engine/Events/EventManager.hpp"
#include "Engine/Events/InputEvents.hpp"

using namespace bee;

Engine::Engine()
{
}

Engine::~Engine()
{
    shutdown();
}

bool Engine::initialize()
{
    Logger::Setup(true);
    LogInfo("Bee Engine initializing... | Ver. {}.{}.{}", BEE_VERSION_MAJOR, BEE_VERSION_MINOR, BEE_VERSION_PATCH);
    
    return true;
}

void Engine::shutdown()
{
    
    LogInfo("Bee Engine shutdown.");
}
