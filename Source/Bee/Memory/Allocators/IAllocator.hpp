/**
 * @File IAllocator.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

namespace bee {

class BEE_API IAllocator
{
public:
    virtual ~IAllocator() = default;

    virtual void* allocate(Size size, Size alignment) = 0;
    virtual void* allocate(Size size, Size alignment, const char* file, const char* line, const char* label = nullptr) = 0;
    
    virtual void deallocate(void* ptr) = 0; 
};
} // namespace bee