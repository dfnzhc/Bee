/**
 * @File Concepts.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/20
 * @Brief This file is part of Bee.
 */

#pragma once

#include <concepts>

namespace Bee
{
    template <typename T, typename... Ts>
    concept AllTypesAreSame = std::conjunction_v<std::is_same<T, Ts>...>;

    template <class>
    constexpr bool AlwaysFalse = false;
    
} // namespace Bee
