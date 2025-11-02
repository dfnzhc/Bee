/**
 * @File Utils.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/30
 * @Brief This file is part of Bee.
 */

#pragma once

namespace Bee
{
    template <class... Ts>
    struct Overloaded : Ts...
    {
        using Ts::operator()...;
    };

    template <class... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;
    
} // namespace Bee
