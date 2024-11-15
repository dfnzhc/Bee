/**
 * @File Core.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/13
 * @Brief This file is part of Bee.
 */

#pragma once






// -------------------------

#include "Base/Defines.hpp"
#include "Base/Portability.hpp"
#include "Base/Assert.hpp"
#include "Base/Logger.hpp"

namespace bee {

inline int Test(int a, int b)
{
    return a + b;
}

BEE_API int Test2(int a, int b);

} // namespace bee
