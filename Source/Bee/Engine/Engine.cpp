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

bool Engine::preInitialize()
{
    LogInfo("Bee Engine pre-initializing... | Ver. {}.{}.{}", BEE_VERSION_MAJOR, BEE_VERSION_MINOR, BEE_VERSION_PATCH);

    return true;
}

bool Engine::initialize(EngineConfig config)
{
    
    _config = std::move(config);
    
    return true;
}

void Engine::shutdown()
{
    
    LogInfo("Bee Engine shutdown.");
}

void Engine::onInputEvent(Ptr<class InputEventBase> event) const
{
    // LogInfo("Event: {}", event->toString().c_str());
}
