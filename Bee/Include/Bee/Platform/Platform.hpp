/**
 * @File Platform.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Bee/Core/Defines.hpp"
#include "Bee/Core/Log.hpp"

namespace bee
{

class Platform
{
public:
    static bool Initialize();
    static void Shutdown();

};

} // namespace bee
