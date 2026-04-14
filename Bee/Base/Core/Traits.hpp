/**
 * @File Traits.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include <concepts>
#include <type_traits>

namespace bee
{

template <class>
constexpr bool AlwaysFalse = false;

} // namespace bee
