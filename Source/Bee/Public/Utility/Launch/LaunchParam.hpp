/**
 * @File LaunchParam.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/20
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Application.hpp"

namespace bee {

using AppCreateFunc = std::function<UniquePtr<Application>()>;

struct BeeLaunchParam
{
    AppCreateFunc createFunc;
};

} // namespace bee