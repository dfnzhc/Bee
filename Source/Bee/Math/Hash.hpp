/**
 * @File Hash.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/12/2
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Math/Math.hpp"

namespace bee {

// TODO: GPU？
template <typename T>
BEE_INLINE BEE_CONSTEXPR void HashCombine(size_t& seed, const T& val)
{
    using std::hash;
    seed ^= hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace bee