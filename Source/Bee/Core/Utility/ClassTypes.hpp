/**
 * @File ClassTypes.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/23
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

namespace bee {
class BEE_API CNonCopyMoveable
{
public:
    CNonCopyMoveable()          = default;
    virtual ~CNonCopyMoveable() = default;

    CNonCopyMoveable(const CNonCopyMoveable&)            = delete;
    CNonCopyMoveable& operator=(const CNonCopyMoveable&) = delete;

    CNonCopyMoveable(CNonCopyMoveable&&)            = delete;
    CNonCopyMoveable& operator=(CNonCopyMoveable&&) = delete;
};

class BEE_API CMoveable
{
public:
    CMoveable()          = default;
    virtual ~CMoveable() = default;

    CMoveable(const CMoveable&)            = delete;
    CMoveable& operator=(const CMoveable&) = delete;

    CMoveable(CMoveable&&)            = default;
    CMoveable& operator=(CMoveable&&) = default;
};
} // namespace bee