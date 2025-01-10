/**
 * @File Memory.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/12/6
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

#include <memory>
#include <optional>
#include <functional>
#include <vector>

namespace bee {

// clang-format off

template<class T> using RawPtr      = T*;
template<class T> using Ptr         = std::shared_ptr<T>;
template<class T> using WeakPtr     = std::weak_ptr<T>;
template<class T> using UniquePtr   = std::unique_ptr<T>;
template<class T> using StdRef      = std::reference_wrapper<T>;
template<class T> using Opt         = std::optional<T>;

template<class T> using RawPtrs     = std::vector<RawPtr<T>>;
template<class T> using Ptrs        = std::vector<Ptr<T>>;
template<class T> using WeakPtrs    = std::vector<WeakPtr<T>>;
template<class T> using UniquePtrs  = std::vector<UniquePtr<T>>;
template<class T> using StdRefs     = std::vector<StdRef<T>>;
template<class T> using Opts        = std::vector<Opt<T>>;
// clang-format on

} // namespace bee
