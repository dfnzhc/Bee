/**
 * @File PlainMemorySource.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/27
 * @Brief This file is part of Bee.
 */

#include "Core/Memory/MemorySources/PlainMemorySource.hpp"
#include "Core/Math/Bits.hpp"

#include <mimalloc.h>

#include "Core/Error.hpp"

using namespace bee;

void* PlainMemorySource::allocate(Size size, Size alignment)
{
    if (!IsPowerOfTwo(alignment)) {
        // TODO: ToNextPowerOfTwo
        alignment = alignof(std::max_align_t);
    }

    if (size == 0) {
        return nullptr;
    }

    void* ptr = mi_malloc_aligned(size, alignment);
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