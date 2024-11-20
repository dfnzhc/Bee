/**
 * @File LaunchParam.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/20
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Base/Application.hpp>
#include <functional>
#include <memory>

namespace bee {

using AppCreateFunc = std::function<std::unique_ptr<Application>()>;

struct BeeLaunchParam
{
    AppCreateFunc createFunc;
};

} // namespace bee