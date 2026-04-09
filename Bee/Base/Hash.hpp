/**
 * @File Hash.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/23
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Config.hpp"

namespace bee
{

[[nodiscard]] constexpr u64 Splitmix64(u64 state) noexcept
{
    auto z = state;
    z      = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z      = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

} // namespace bee
