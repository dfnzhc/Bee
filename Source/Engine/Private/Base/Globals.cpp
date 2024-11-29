/**
 * @File Globals.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#include "Base/Globals.hpp"

using namespace bee;

namespace  {

class GlobalState
{
public:
    bool IsRequestingExit = false;
    
    // TODO：添加控制
    bool IsVerbosePrintEnable = true;
    
    bool IsValidationLayerEnable = true;
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

bool Globals::IsPrintVerboseEnabled()
{
    return globals.IsVerbosePrintEnable;
}

bool Globals::EnableValidationLayer()
{
    return globals.IsValidationLayerEnable;
}
