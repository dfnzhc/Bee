/**
 * @File Memory.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/3
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include <memory>

namespace bee {
/// ==========================

// clang-format off
template<class T> using RawPtr      = T*;
template<class T> using Ptr         = std::shared_ptr<T>;
template<class T> using WeakPtr     = std::weak_ptr<T>;
template<class T> using UniquePtr   = std::unique_ptr<T>;
template<class T> using StdRef      = std::reference_wrapper<T>;
// clang-format on

template<typename T, typename... Args>
    requires std::is_constructible_v<T, Args...>
auto MakePtr(Args&&... args) -> Ptr<T>
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
    requires std::is_constructible_v<T, Args...>
auto MakeUnique(Args&&... args) -> Ptr<T>
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

/// ==========================

#ifdef BEE_ENABLE_DEBUG
#define BEE_MEMORY_DEBUG
#endif

enum class MemoryUsage : u32
{
    Unknown = 0,
};

/// ==========================

/**
 * @tparam Level Indicates the magnitude of the memory size.
 * @tparam Mul Numeric multiplier between two levels.
 * @param sz Memory size at this level and multiplier.
 * @return Memory size in byte.
 */
template<u8 Level, Size Mul>
BEE_API constexpr Size MemorySize(Size sz)
{
    if constexpr (Level == 0)
        return sz;
    else {
        return Mul * MemorySize<Level - 1, Mul>(sz);
    }
}

// clang-format off

BEE_API constexpr Size KB(Size sz) { return MemorySize<1, 1000z>(sz); }
BEE_API constexpr Size MB(Size sz) { return MemorySize<2, 1000z>(sz); }
BEE_API constexpr Size GB(Size sz) { return MemorySize<3, 1000z>(sz); }

BEE_API constexpr Size KiB(Size sz) { return MemorySize<1, 1024z>(sz); }
BEE_API constexpr Size MiB(Size sz) { return MemorySize<2, 1024z>(sz); }
BEE_API constexpr Size GiB(Size sz) { return MemorySize<3, 1024z>(sz); }

// clang-format on


/// ==========================

constexpr Size kMaxAllocationSize = GiB(16);
constexpr Size kMaxDynamicSize    = MiB(32);


/// ==========================

/**
 * @brief Allocates a block of memory of specified size.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated memory, or nullptr on failure.
 */
BEE_API void* Malloc(Size size);

/**
 * @brief Allocates and zero-initializes an array of elements.
 * @param count Number of elements.
 * @param size Size of each element.
 * @return Pointer to the allocated memory, or nullptr on failure.
 */
BEE_API void* Calloc(Size count, Size size);

/**
 * @brief Resizes a previously allocated memory block.
 * @param ptr Pointer to the original memory block.
 * @param newSize New size in bytes.
 * @return Pointer to the resized memory block, or nullptr on failure.
 */
BEE_API void* Realloc(void* ptr, Size newSize);

/**
 * @brief Reallocates and zero-initializes an array, adjusting element count and size.
 * @param ptr Pointer to the original memory block.
 * @param newCount Number of elements.
 * @param size Size of each element.
 * @return Pointer to the reallocated memory, or NULL on failure.
 */
BEE_API void* Recalloc(void* ptr, Size newCount, Size size);

/**
 * @brief Deallocates a memory block.
 * @param ptr Pointer to the memory block to free.
 */
BEE_API void Free(void* ptr);

/**
 * @brief Expands memory block without relocating it.
 * @param ptr Pointer to the original memory block.
 * @param newSize New size in bytes.
 * @return Pointer to the expanded block, or nullptr on failure.
 */
BEE_API void* Expand(void* ptr, Size newSize);

/**
 * @brief Returns the usable size of an allocated memory block.
 * @param ptr Pointer to the memory block.
 * @return Size of the usable memory region in bytes.
 */
BEE_API Size UsableSize(const void* ptr);

/**
 * @brief Safely allocates and copies an environment variable's value.
 * @param buf Buffer to store the result (allocated if nullptr
 * @param size Size of the buffer.
 * @param name Name of the environment variable.
 * @return Error code (0 on success).
 */
BEE_API int DumpEnv(char** buf, Size* size, const char* name);

/**
 * @brief Suggests an optimal allocation size for a given request.
 * @param size Requested size in bytes.
 * @return Recommended allocation size.
 */
BEE_API Size MallocGoodSize(Size size);

/**
 * @brief Allocates memory with a specified alignment.
 * @param size Number of bytes to allocate.
 * @param alignment Alignment requirement.
 * @return Pointer to the aligned memory, or nullptr on failure.
 */
BEE_API void* MallocAligned(Size size, Size alignment);

/**
 * @brief Reallocates aligned memory.
 * @param ptr Original aligned memory block.
 * @param newSize New size in bytes.
 * @param alignment Alignment requirement.
 * @return Pointer to the reallocated aligned memory, or nullptr on failure.
 */
BEE_API void* ReallocAligned(void* ptr, Size newSize, Size alignment);

/**
 * @brief Reallocates and zero-initializes an aligned array.
 * @param ptr Original aligned memory block.
 * @param newCount Number of elements.
 * @param size Size of each element.
 * @param alignment Alignment requirement.
 * @return Pointer to the reallocated aligned memory, or nullptr on failure.
 */
BEE_API void* RecallocAligned(void* ptr, Size newCount, Size size, Size alignment);

/**
 * @brief Allocates memory aligned at a specific offset within the block.
 * @param size Number of bytes to allocate.
 * @param alignment Alignment requirement.
 * @param offset Offset within the block to enforce alignment.
 * @return Pointer to the aligned memory, or nullptr on failure.
 */
BEE_API void* MallocAlignedAt(Size size, Size alignment, Size offset);

/**
 * @brief Reallocates memory aligned at a specific offset.
 * @param ptr Original aligned memory block.
 * @param newSize New size in bytes.
 * @param alignment Alignment requirement.
 * @param offset Offset within the block to enforce alignment.
 * @return Pointer to the reallocated aligned memory, or nullptr on failure.
 */
BEE_API void* ReallocAlignedAt(void* ptr, Size newSize, Size alignment, Size offset);

/**
 * @brief Reallocates and zero-initializes an array aligned at a specific offset.
 * @param ptr Original aligned memory block.
 * @param newCount Number of elements.
 * @param size Size of each element.
 * @param alignment Alignment requirement.
 * @param offset Offset within the block to enforce alignment.
 * @return Pointer to the reallocated aligned memory, or nullptr on failure.
 */
BEE_API void* RecallocAlignedAt(void* ptr, Size newCount, Size size, Size alignment, Size offset);

/// ==========================
} // namespace bee