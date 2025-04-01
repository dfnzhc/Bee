/**
 * @File Launch.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/1
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Base/Error.hpp"

#include <functional>

namespace bee {

class Application;
using ApplicationCreator = std::function<std::unique_ptr<Application>()>;

struct LaunchParam
{
    ApplicationCreator appCreator;

    // TODO: Add program execution params?
};

BEE_API extern int Launch(const LaunchParam& launchParam);
BEE_API extern void Shutdown();

} // namespace bee
