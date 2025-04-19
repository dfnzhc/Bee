/**
 * @File Property.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/13
 * @Brief This file is part of Bee.
 */

#include "Core/App/Property.hpp"

using namespace bee;

namespace  {

bool bIsEngineRunning = true;

} // namespace 

bool Property::IsEngineRunning()
{
    return bIsEngineRunning;
}

void Property::RequestEngineExit()
{
    bIsEngineRunning = false;
}