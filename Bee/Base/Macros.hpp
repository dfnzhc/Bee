/**
 * @File Macros.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/17
 * @Brief This file is part of Bee.
 */

#pragma once

#define BEE_ARRAY_SIZE(array) (sizeof(::bee::internal::ArraySizeHelper(array)))

namespace bee::internal
{

template <typename T, size_t N>
auto ArraySizeHelper(const T (&array)[N]) -> char (&)[N];

} // namespace bee::internal
