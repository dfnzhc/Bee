/**
 * @File Common.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/28
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Globals.hpp"
#include <Utility/Logger.hpp>

namespace bee {

#define LogVerbose(...)                                                                                                                              \
    do {                                                                                                                                             \
        if (Globals::IsPrintVerboseEnabled()) {                                                                                                      \
            LogInfo(__VA_ARGS__);                                                                                                                    \
        }                                                                                                                                            \
    } while (false)

} // namespace bee