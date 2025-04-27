/**
 * @File PlainAllocator.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Memory/Allocators/IAllocator.hpp"

namespace bee {
/**
 * A plain allocator, basically a wrapper for mimalloc, directly using mi_malloc, mi_free and so on.
 */
class BEE_API PlainAllocator : public IAllocator
{
public:
    BEE_NODISCARD void* allocate(Size size, Size alignment = 0) override;
    void deallocate(void* ptr) override;

    BEE_NODISCARD Size usableSize(void* ptr) override;
    BEE_NODISCARD bool owns(void* ptr) const override;
    BEE_NODISCARD const char* name() const override;
};
} // namespace bee