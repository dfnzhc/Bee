/**
 * @File PlainAllocator.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/27
 * @Brief This file is part of Bee.
 */

#include "Memory/Allocators/PlainAllocator.hpp"
#include <mimalloc.h>

using namespace bee;

void* PlainAllocator::allocate(Size size, Size alignment)
{
    if (alignment == 0)
        return mi_malloc(size);

    return mi_malloc_aligned(size, alignment);
}

void PlainAllocator::deallocate(void* ptr)
{
    if (owns(ptr))
        mi_free_size(ptr, mi_usable_size(ptr));
}

Size PlainAllocator::usableSize(void* ptr)
{
    if (owns(ptr))
        return mi_usable_size(ptr);

    return 0;
}

bool PlainAllocator::owns(void* ptr) const
{
    return ptr && mi_check_owned(ptr);
}

const char* PlainAllocator::name() const
{
    return "Plain";
}