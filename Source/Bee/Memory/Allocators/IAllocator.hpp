/**
 * @File IAllocator.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Portability.hpp"

namespace bee {

class BEE_API IAllocator
{
public:
    virtual ~IAllocator() = default;

    /**
     * @brief Allocates a block of memory with the specified size and alignment.
     *
     * @param size The desired size of the memory block in bytes.
     * @param alignment The desired alignment of the memory block in bytes. Must be a power of two.
     * @return A pointer to the allocated memory block on success, or nullptr on failure (e.g., out of memory, invalid parameters).
     */
    BEE_NODISCARD virtual void* allocate(Size size, Size alignment = 0) = 0;

    /**
     * @brief Deallocates a previously allocated block of memory.
     *
     * @param ptr A pointer to the memory block to deallocate.
     */
    virtual void deallocate(void* ptr) = 0;

    /**
     * @brief Return the available bytes in a memory block.
     *
     * @param ptr Pointer to allocated memory.
     * @return The available bytes in the memory block, or 0 if p was nullptr or not allocated from the allocator.
     */
    BEE_NODISCARD virtual Size usableSize(void* ptr) = 0;

    /**
     * @brief Check weather the memory block allocated from this allocator.
     *
     * @param ptr A pointer to the memory block.
     */
    BEE_NODISCARD virtual bool owns(void* ptr) const = 0;

    /**
     * @return The name of the allocator.
     */
    BEE_NODISCARD virtual const char* name() const = 0;
};
} // namespace bee