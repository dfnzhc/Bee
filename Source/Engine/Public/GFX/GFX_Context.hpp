/**
 * @File GFX_Context.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>
#include <Core/Portability.hpp>
#include <Utility/Error.hpp>
#include <Utility/String.hpp>
#include <Memory/Memory.hpp>

#include "GFX/RHI/RHI_Enum.hpp"

namespace bee {
class GFX_DeviceDriver;

class BEE_API GFX_Context
{
public:
    struct DeviceInfo
    {
        String name     = "Unknown";
        Vendor vendor   = Vendor::Unknown;
        DeviceType type = DeviceType::Other;
    };

    struct Config
    {
        bool headless = false;
    };

    virtual ~GFX_Context() = default;

    // clang-format off
    virtual Error create(const Config&) = 0;
    virtual void destroy() = 0;

    BEE_NODISCARD virtual const DeviceInfo& deviceInfo(u32 devIdx) const = 0;
    BEE_NODISCARD virtual u32 deviceCount() const                        = 0;

    BEE_NODISCARD virtual UniquePtr<GFX_DeviceDriver> createDriver() = 0;
    // clang-format on
};
} // namespace bee