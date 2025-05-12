/**
 * @File Defines.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#pragma once

// -------------------------

#if defined(_MSC_VER)
#  define BEE_API_EXPORT __declspec(dllexport)
#  define BEE_API_IMPORT __declspec(dllimport)
#elif defined(__clang__) || defined(__GNUC__)
#  define BEE_API_EXPORT __attribute__((visibility("default")))
#  define BEE_API_IMPORT __attribute__((visibility("default")))
#else
#  define BEE_API_EXPORT
#  define BEE_API_IMPORT
#endif

#ifdef BEE_DLL
#  define BEE_API BEE_API_EXPORT
#else  // BEE_DLL
#  define BEE_API BEE_API_IMPORT
#endif // BEE_DLL

#include "Core/Math/MathDefines.hpp"

#include <optional>

namespace bee {
template<typename T>
using Opt = std::optional<T>;

template<typename A, typename B>
using OptPair = Opt<std::pair<A, B>>;
} // namespace bee