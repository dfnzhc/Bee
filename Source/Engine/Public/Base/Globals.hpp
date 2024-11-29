/**
 * @File Globals.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>
#include <memory>

namespace bee {

class BEE_API Globals
{
public:
    
    // -------------------------
    // 全局
    // -------------------------
    static bool IsEngineExitRequested();
    static void RequestEngineExit();
    
    static bool IsPrintVerboseEnabled();

    // -------------------------
    // 渲染相关
    // -------------------------
    static bool EnableValidationLayer();
    
};


} // namespace bee