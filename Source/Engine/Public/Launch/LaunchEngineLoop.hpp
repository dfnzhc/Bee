/**
 * @File LaunchEngineLoop.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>
#include <memory>

#include "Launch/LaunchParam.hpp"

namespace bee {

class Application;

class BEE_API EngineLoop
{
private:
    EngineLoop() = default;
    ~EngineLoop() = default;
    
public:
    static EngineLoop& Instance()
    {
        static EngineLoop loop;
        return loop;
    }
    
    int preInit(const BeeLaunchParam& param);
    int init();
    
    void tick();
    void shutdown();
};

} // namespace bee