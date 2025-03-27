/**
 * @File Logger.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"

#define QUILL_DISABLE_NON_PREFIXED_MACROS
#include <quill/core/Attributes.h>
#include <quill/Logger.h>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>

#include <quill/std/Array.h>
#include <quill/std/Chrono.h>
#include <quill/std/Deque.h>
#include <quill/std/FilesystemPath.h>
#include <quill/std/ForwardList.h>
#include <quill/std/List.h>
#include <quill/std/Map.h>
#include <quill/std/Optional.h>
#include <quill/std/Pair.h>
#include <quill/std/Set.h>
#include <quill/std/Tuple.h>
#include <quill/std/UnorderedMap.h>
#include <quill/std/UnorderedSet.h>
#include <quill/std/Vector.h>
#include <quill/std/WideString.h>

namespace bee {
class BEE_API Logger
{
public:
    static void Setup(bool buildFileLogger = true);

    enum class Type
    {
        Console, File
    };

    static quill::Logger* Get(Type type);
};
} // namespace bee

// clang-format off
#define LogTrace3(fmt, ...)                                                 \
    do {                                                                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::Console))    \
            QUILL_LOG_TRACE_L3(logger, fmt, ##__VA_ARGS__);                 \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::File))       \
            QUILL_LOG_TRACE_L3(logger, fmt, ##__VA_ARGS__);                 \
    } while(0)

#define LogTrace2(fmt, ...)                                                 \
    do {                                                                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::Console))    \
            QUILL_LOG_TRACE_L2(logger, fmt, ##__VA_ARGS__);                 \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::File))       \
            QUILL_LOG_TRACE_L2(logger, fmt, ##__VA_ARGS__);                 \
    } while(0)
    
#define LogTrace1(fmt, ...)                                                 \
    do {                                                                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::Console))    \
            QUILL_LOG_TRACE_L1(logger, fmt, ##__VA_ARGS__);                 \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::File))       \
            QUILL_LOG_TRACE_L1(logger, fmt, ##__VA_ARGS__);                 \
    } while(0)

#define LogDebug(fmt, ...)                                                  \
    do {                                                                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::Console))    \
            QUILL_LOG_DEBUG(logger, fmt, ##__VA_ARGS__);                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::File))       \
            QUILL_LOG_DEBUG(logger, fmt, ##__VA_ARGS__);                    \
    } while(0)

#define LogInfo(fmt, ...)                                                   \
    do {                                                                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::Console))    \
            QUILL_LOG_INFO(logger, fmt, ##__VA_ARGS__);                     \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::File))       \
            QUILL_LOG_INFO(logger, fmt, ##__VA_ARGS__);                     \
    } while(0)

#define LogWarn(fmt, ...)                                                   \
    do {                                                                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::Console))    \
            QUILL_LOG_WARNING(logger, fmt, ##__VA_ARGS__);                  \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::File))       \
            QUILL_LOG_WARNING(logger, fmt, ##__VA_ARGS__);                  \
    } while(0)
    
#define LogError(fmt, ...)                                                  \
    do {                                                                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::Console))    \
            QUILL_LOG_ERROR(logger, fmt, ##__VA_ARGS__);                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::File))       \
            QUILL_LOG_ERROR(logger, fmt, ##__VA_ARGS__);                    \
    } while(0)

#define LogFatal(fmt, ...)                                                  \
    do {                                                                    \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::Console))    \
            QUILL_LOG_CRITICAL(logger, fmt, ##__VA_ARGS__);                 \
        if (auto* logger = bee::Logger::Get(bee::Logger::Type::File))       \
            QUILL_LOG_CRITICAL(logger, fmt, ##__VA_ARGS__);                 \
    } while(0)

// clang-format on