/**
 * @File Memory.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/3
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Portability.hpp"

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

} // namespace bee