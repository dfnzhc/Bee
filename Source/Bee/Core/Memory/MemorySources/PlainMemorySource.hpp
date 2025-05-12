/**
 * @File PlainMemorySource.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Memory/MemorySources/IMemorySource.hpp"

namespace bee {
/**
 * @brief Basically a wrapper for mimalloc.
 */
class BEE_API PlainMemorySource : public IMemorySource
{
public:
    PlainMemorySource()           = default;
    ~PlainMemorySource() override = default;

    void* allocate(Size size, Size alignment = 0) override;
    void deallocate(void* ptr, Size size) noexcept override;

    AllocStats stats() const noexcept override;

private:
    AllocStats _stats;
};
} // namespace bee