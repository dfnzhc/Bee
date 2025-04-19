/**
 * @File Macros.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/1/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Portability.hpp"

#undef RGB
#undef ERROR

#define BEE_BIT(bit) (1 << (bit))

#define SimpleCheckAndReturn(expr) do { if (!(expr)) { return false; } } while(false)
