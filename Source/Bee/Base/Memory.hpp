/**
 * @File Memory.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/3
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Base/Defines.hpp"
#include <memory>

namespace bee {

// clang-format off
template<class T> using RawPtr      = T*;
template<class T> using Ptr         = std::shared_ptr<T>;
template<class T> using WeakPtr     = std::weak_ptr<T>;
template<class T> using UniquePtr   = std::unique_ptr<T>;
template<class T> using StdRef      = std::reference_wrapper<T>;
// clang-format on

using VoidPtr = RawPtr<void>;

} // namespace bee