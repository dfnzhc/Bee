/**
 * @File Storage.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/16
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Core/Defines.hpp"

namespace bee {
class BEE_API Property final
{
public:

    /// ==========================
    /// Engine Functions
    /// ==========================
    static bool IsEngineRunning();
    static void RequestEngineExit();
};
} // namespace bee
