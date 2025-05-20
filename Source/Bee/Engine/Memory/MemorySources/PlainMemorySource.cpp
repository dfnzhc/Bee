/**
 * @File PlainMemorySource.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/27
 * @Brief This file is part of Bee.
 */

#include "Memory/MemorySources/PlainMemorySource.hpp"
#include "Error.hpp"
#include "Math/Bits.hpp"

#include <mimalloc.h>

using namespace bee;

void* PlainMemorySource::allocate(Size size, Size alignment)
{
    if (size == 0) {
        return nullptr;
    }
    
    if (alignment != 0 && !IsPowerOfTwo(alignment)) {
        alignment = NextPowerOfTwo(sizeof(std::max_align_t));
    }

    void* ptr = nullptr;

    if (alignment == 0) {
        ptr = mi_malloc(size);
    }
    else {
        ptr = mi_malloc_aligned(size, alignment);
    }
    
    if (ptr != nullptr) {
        _stats.alloc(size);
    }

    return ptr;
}

void PlainMemorySource::deallocate(void* ptr, Size size) noexcept
{
    // pointer must not null and allocated from this memory source.
    if (ptr == nullptr || !mi_check_owned(ptr)) {
        return;
    }

    BEE_ASSERT(size == mi_usable_size(ptr), "Deallocated size not equals to the allocated size.");
    
    _stats.dealloc(size);
    mi_free_size(ptr, size);
}

AllocStats PlainMemorySource::stats() const noexcept
{
    return _stats;
}