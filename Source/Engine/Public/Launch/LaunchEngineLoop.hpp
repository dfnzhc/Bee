/**
 * @File LaunchLoop.hpp
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

class BEE_API LaunchLoop
{
private:
    LaunchLoop() = default;
    ~LaunchLoop() = default;
    
public:
    static LaunchLoop& Instance()
    {
        static LaunchLoop loop;
        return loop;
    }
    
    int preInit(const BeeLaunchParam& param);
    int init();
    
    void tick();
    void shutdown();
};

} // namespace bee