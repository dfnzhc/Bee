/**
 * @File MemoryManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/23
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Memory/Memory.hpp"
#include "Memory/Allocator.hpp"
#include "Memory/MemoryPool.hpp"
#include "Memory/AllocStats.hpp"

#include "Core/Concepts/ClassTypes.hpp"

namespace bee {

class BEE_API MemoryManager : public CSingleton<MemoryManager>
{
protected:
    ~MemoryManager() override;
};

} // namespace bee