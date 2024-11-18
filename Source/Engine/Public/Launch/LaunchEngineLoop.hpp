/**
 * @File LaunchEngineLoop.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>

namespace bee {

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
    
    int PreInit();
    int Init();
    
    void Tick();

    void Exit();
    
protected:

};

} // namespace bee