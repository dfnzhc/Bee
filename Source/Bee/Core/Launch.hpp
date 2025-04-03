/**
 * @File Launch.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/1
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Base/Error.hpp"
#include "Base/Memory.hpp"

#include <functional>

namespace bee {

class Engine;
using EngineCreator = std::function<UniquePtr<Engine>()>;

struct LaunchParam
{
    EngineCreator engineCreator;

    // TODO: Add program execution params?
};

BEE_API extern int Launch(const LaunchParam& launchParam);
BEE_API extern void Shutdown();

} // namespace bee
