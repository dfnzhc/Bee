/**
 * @File PlainMemorySource.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Memory/MemorySources/IMemorySource.hpp"

namespace bee {
/**
 * @brief An IMemorySource implementation that uses the mimalloc 's functionalities
 */
class BEE_API MallocMemorySource : public IMemorySource
{
public:
    MallocMemorySource()           = default;
    ~MallocMemorySource() override = default;

    void* allocate(Size size, Size alignment) override;
    void deallocate(void* ptr, Size size) noexcept override;

    AllocStats stats() const noexcept override;

private:
    AllocStats _stats;
};
} // namespace bee