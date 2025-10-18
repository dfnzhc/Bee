/**
 * @File PlatformTypes.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Base/Defines.hpp"

namespace Bee
{
    // 平台类型句柄
    #define XIHE_PLATFORM_HANDLE(HandleName, UnderlyingType)                                    \
        struct HandleName {                                                                     \
            UnderlyingType handle;                                                              \
            explicit HandleName(UnderlyingType h = {}) : handle(h) {}                           \
            bool operator==(const HandleName& other) const { return handle == other.handle; }   \
            bool operator!=(const HandleName& other) const { return handle != other.handle; }   \
            bool operator<(const HandleName& other) const { return handle < other.handle; }     \
            explicit operator bool() const { return handle != UnderlyingType{}; }               \
        }

    // 显示器信息
    struct DisplayInfo
    {
        std::string name = {};
        i32 index        = -1;
        i32 posX         = 0;
        i32 posY         = 0;
        u32 width        = 0;
        u32 height       = 0;
        f32 dpi          = 1.0f;
        f32 refreshRate  = 60.0f;
        bool isPrimary   = false;
    };
} // namespace Bee
