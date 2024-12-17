/**
 * @File Macros.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/20
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Portability.hpp"

#define BEE_CLASS_DELETE_COPY(ClassName)                                                                                                             \
    ClassName(const ClassName&)            = delete;                                                                                                 \
    ClassName& operator=(const ClassName&) = delete

#define BEE_CLASS_DELETE_MOVE(ClassName)                                                                                                             \
    ClassName(ClassName&&)            = delete;                                                                                                      \
    ClassName& operator=(ClassName&&) = delete

#define BEE_CLASS_MOVABLE_ONLY(ClassName)                                                                                                            \
    ClassName(const ClassName&)                = delete;                                                                                             \
    ClassName& operator=(const ClassName&)     = delete;                                                                                             \
    ClassName(ClassName&&) noexcept            = default;                                                                                            \
    ClassName& operator=(ClassName&&) noexcept = default