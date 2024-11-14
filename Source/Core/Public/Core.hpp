/**
 * @File Core.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include "./Defines.hpp"
#include "./Portability.hpp"
#include "./Assert.hpp"
#include "./Logger.hpp"

namespace bee {

inline int Test(int a, int b)
{
    return a + b;
}

BEE_API int Test2(int a, int b);

} // namespace bee
