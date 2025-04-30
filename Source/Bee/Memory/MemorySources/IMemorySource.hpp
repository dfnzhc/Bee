/**
 * @File IMemorySource.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Concepts/ClassTypes.hpp"

#include "Memory/AllocStats.hpp"

namespace bee {
/**
 * @brief Interface for acquiring and releasing raw memory blocks from an underlying source.
 * This interface abstracts the actual origin of the memory (e.g., OS heap, specific GPU memory heap).
 * Allocator implementations (like PoolAllocator, LinearAllocator) will use an IMemorySource
 * to get the large chunks of memory they manage and sub-allocate from.
 */
class BEE_API IMemorySource : public CNonCopyMoveable
{
public:
    ~IMemorySource() override = default;

    /**
     * @brief Allocate a raw block of memory.
     * @param size The number of bytes to allocate.
     * @param alignment The required alignment of the memory block. Must be a power of two. Defaults to the maximum fundamental alignment.
     * @return A pointer to the allocated memory block, or nullptr if allocation fails.
     *         The allocated block size will be at least 'size' bytes.
     */
    virtual void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;

    /**
     * @brief Deallocate a raw block of memory previously allocated by this source.
     * @param ptr Pointer to the memory block to deallocate. If ptr is nullptr, the function does nothing.
     * @param size The size of the memory block that was requested during allocation.
     *             This parameter might be required by some underlying OS/API deallocation functions.
     *             It should match the size passed to the allocate call that returned ptr.
     */
    virtual void deallocate(void* ptr, size_t size) noexcept = 0;

    /**
     * @brief Get the current allocation statistics for this memory source.
     * @return An AllocationStats structure containing the statistics. The returned values represent a snapshot at the time of the call.
     */
    virtual AllocStats stats() const noexcept = 0;
};
} // namespace bee