/**
 * @File LogMessage.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include <string>
#include <string_view>
#include <source_location>
#include "Core/Base/Defines.hpp"

namespace Bee
{
    struct LogMessage
    {
        enum class Level : u8
        {
            Trace = 0,
            Info  = 1,
            Warn  = 2,
            Error = 3,
            Fatal = 4
        };

        Level level;
        std::string message;
        std::source_location location;

        LogMessage(Level l, std::string_view msg, std::source_location loc = std::source_location::current()) :
            level(l), message(msg), location(loc)
        {
        }
    };
} // namespace Bee
