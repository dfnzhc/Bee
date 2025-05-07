/**
 * @File MemoryManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/23
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Core/Memory/Memory.hpp"
#include "Core/Memory/Allocator.hpp"
#include "Core/Memory/MemoryPool.hpp"
#include "Core/Memory/AllocStats.hpp"

#include "Core/Utility/ClassTypes.hpp"

namespace bee {

class BEE_API MemoryManager : public CSingleton<MemoryManager>
{
protected:
    ~MemoryManager() override;
};

} // namespace bee