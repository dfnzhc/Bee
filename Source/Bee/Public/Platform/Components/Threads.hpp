/**
 * @File Threads.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include <thread>

namespace bee {

enum class ThreadPriorityType : int
{
    BackgroundBegin = -2, ///< Indicates I/O-intense thread
    BackgroundEnd   = -1, ///< Indicates the end of I/O-intense operations in the thread
    Lowest          = 0,  ///< Lowest priority
    Low             = 1,
    Normal          = 2,
    High            = 3,
    Highest         = 4,
};

} // namespace bee