/**
 * @File Property.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/13
 * @Brief This file is part of Bee.
 */

#include "Core/Property.hpp"

using namespace bee;

namespace  {

bool bIsEngineRunning = false;

} // namespace 

void Property::SetEngineRunning(bool bIsRunning)
{
    bIsEngineRunning = bIsRunning;
}

bool Property::IsEngineRunning()
{
    return bIsEngineRunning;
}