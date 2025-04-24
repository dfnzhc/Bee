/**
 * @File Memory.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/23
 * @Brief This file is part of Bee.
 */

#include "Memory/Memory.hpp"

#include <mimalloc.h>
#include <mimalloc-override.h>
#include <mimalloc-new-delete.h>
#include <mimalloc-stats.h>

using namespace bee;


void* bee::Malloc(Size size)
{
    return mi_malloc(size);
}

void* bee::Calloc(Size count, Size size)
{
    return mi_calloc(count, size);
}

void* bee::Realloc(void* ptr, Size newSize)
{
    return mi_realloc(ptr, newSize);
}

void* bee::Recalloc(void* ptr, Size newCount, Size size)
{
    return mi_recalloc(ptr, newCount, size);
}

void bee::Free(void* ptr)
{
    return mi_free(ptr);
}

void* bee::Expand(void* ptr, Size newSize)
{
    return mi_expand(ptr, newSize);
}

Size bee::UsableSize(const void* ptr)
{
    return mi_usable_size(ptr);
}

int bee::DumpEnv(char** buf, Size* size, const char* name)
{
    return mi_dupenv_s(buf, size, name);
}

Size bee::MallocGoodSize(Size size)
{
    return mi_malloc_good_size(size);
}

void* bee::MallocAligned(Size size, Size alignment)
{
    return mi_malloc_aligned(size, alignment);
}

void* bee::ReallocAligned(void* ptr, Size newSize, Size alignment)
{
    return mi_realloc_aligned(ptr, newSize, alignment);
}

void* bee::RecallocAligned(void* ptr, Size newCount, Size size, Size alignment)
{
    return mi_recalloc_aligned(ptr, newCount, size, alignment);
}

void* bee::MallocAlignedAt(Size size, Size alignment, Size offset)
{
    return mi_malloc_aligned_at(size, alignment, offset);
}

void* bee::ReallocAlignedAt(void* ptr, Size newSize, Size alignment, Size offset)
{
    return mi_realloc_aligned_at(ptr, newSize, alignment, offset);
}

void* bee::RecallocAlignedAt(void* ptr, Size newCount, Size size, Size alignment, Size offset)
{
    return mi_recalloc_aligned_at(ptr, newCount, size, alignment, offset);
}