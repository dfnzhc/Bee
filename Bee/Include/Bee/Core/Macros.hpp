/**
 * @File Macros.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Defines.hpp"

// 用于连接字符
#define BEE_CONCAT_(A, B) A##B
#define BEE_CONCAT(A, B) BEE_CONCAT_(A, B)

#define BEE_UNUSED(x) (void)x

namespace bee
{

} // namespace bee
