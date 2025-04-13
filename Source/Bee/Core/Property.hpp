/**
 * @File Property.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"

namespace bee {

class BEE_API Property final
{
public:
    static void SetEngineRunning(bool bIsRunning);
    static bool IsEngineRunning();
    
};

} // namespace bee
