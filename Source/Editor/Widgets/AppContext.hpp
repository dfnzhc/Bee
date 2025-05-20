/**
 * @File AppContext.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/5/20
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Core/Portability.hpp"

#include <memory>

namespace bee {

class Engine;

class AppContext final
{
public:
    AppContext();
    ~AppContext();
    
    BEE_NODISCARD bool initialize();
    BEE_NODISCARD void shutdown();

    BEE_NODISCARD Engine* engine() const;
    
private:
    std::unique_ptr<Engine> _pEngine;
};


} // namespace bee