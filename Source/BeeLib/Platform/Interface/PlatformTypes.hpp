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
        std::string name  = {};
        uint32_t index    = 0;
        u32 sizeX         = 0;
        u32 sizeY         = 0;
        u32 dpiX          = 96;
        u32 dpiY          = 96;
        float refreshRate = 60.0f;
        bool isPrimary    = false;
    };

} // namespace Bee
