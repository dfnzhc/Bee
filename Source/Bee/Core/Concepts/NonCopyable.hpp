/**
 * @File NonCopyable.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

namespace bee {

class BEE_API NonCopyable
{
public:
    NonCopyable()          = default;
    virtual ~NonCopyable() = default;

    NonCopyable(const NonCopyable&)    = delete;
    void operator=(const NonCopyable&) = delete;
};

} // namespace bee
