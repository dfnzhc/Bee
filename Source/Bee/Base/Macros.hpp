/**
 * @File Macros.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/1/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"
#include "Base/Portability.hpp"

#define BEE_CLASS_DELETE_COPY(ClassName)                                                                                                                                                               \
    ClassName(const ClassName&)            = delete;                                                                                                                                                   \
    ClassName& operator=(const ClassName&) = delete

#define BEE_CLASS_DELETE_MOVE(ClassName)                                                                                                                                                               \
    ClassName(ClassName&&)            = delete;                                                                                                                                                        \
    ClassName& operator=(ClassName&&) = delete

#define BEE_CLASS_MOVABLE_ONLY(ClassName)                                                                                                                                                              \
    ClassName(const ClassName&)                = delete;                                                                                                                                               \
    ClassName& operator=(const ClassName&)     = delete;                                                                                                                                               \
    ClassName(ClassName&&) noexcept            = default;                                                                                                                                              \
    ClassName& operator=(ClassName&&) noexcept = default

#undef RGB
#undef ERROR

#define BEE_BIT(bit) (1 << (bit))
