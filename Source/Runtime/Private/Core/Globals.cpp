/**
 * @File Globals.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#include "Core/Globals.hpp"

using namespace bee;

namespace  {

class GlobalState
{
public:
    bool IsRequestingExit = false;
};

GlobalState globals;

} // namespace 


bool Globals::IsEngineExitRequested()
{
    return globals.IsRequestingExit;
}

void Globals::RequestEngineExit()
{
    globals.IsRequestingExit = true;
}
