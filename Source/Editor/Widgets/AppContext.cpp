/**
 * @File AppContext.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/5/20
 * @Brief This file is part of Bee.
 */

#include "Widgets/AppContext.hpp"

#include "Core/Error.hpp"
#include "Core/Logger.hpp"
#include "Engine/Engine.hpp"

using namespace bee;

AppContext::AppContext()
{
}

AppContext::~AppContext()
{
    shutdown();
}

bool AppContext::initialize()
{
    _pEngine = std::make_unique<Engine>();
    if (!_pEngine->initialize()) {
        LogError("Engine initialization failed!");   
        return false;
    }
    
    return true;
}

void AppContext::shutdown()
{
    if (_pEngine) {
        _pEngine.reset();
        _pEngine = nullptr;
    }
}

Engine* AppContext::engine() const
{
    return _pEngine.get();
}