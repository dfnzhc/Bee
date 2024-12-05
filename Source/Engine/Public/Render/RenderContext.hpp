/**
 * @File RenderContext.hpp
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

namespace bee {

class RenderDriver;

class BEE_API RenderContext
{
public:
    enum Vendor
    {
        Vendor_Unknown   = 0x0,
        Vendor_AMD       = 0x1002,
        Vendor_IMGTEC    = 0x1010,
        Vendor_APPLE     = 0x106B,
        Vendor_NVIDIA    = 0x10DE,
        Vendor_ARM       = 0x13B5,
        Vendor_MICROSOFT = 0x1414,
        Vendor_QUALCOMM  = 0x5143,
        Vendor_INTEL     = 0x8086
    };
    
    enum DeviceType
    {
        DeviceType_Other         = 0x0,
        DeviceType_IntegratedGPU = 0x1,
        DeviceType_DiscreteGPU   = 0x2,
        DeviceType_VirtualGPU    = 0x3,
        DeviceType_CPU           = 0x4,
        DeviceType_MAX           = 0x5
    };

    struct DeviceInfo
    {
        String name     = "Unknown";
        Vendor vendor   = Vendor_Unknown;
        DeviceType type = DeviceType_Other;
    };

    struct Config
    {
        bool headless = false;
    };

    virtual ~RenderContext() = default;

    // clang-format off
    virtual Error create(const Config&) = 0;
    virtual void destroy() = 0;

    BEE_NODISCARD virtual const DeviceInfo& deviceInfo(u32 devIdx) const = 0;
    BEE_NODISCARD virtual u32 deviceCount() const                        = 0;
    BEE_NODISCARD virtual bool deviceSupportsPresent(u32 devIdx) const   = 0;

    BEE_NODISCARD virtual UniquePtr<RenderDriver> createDriver() = 0;
    // clang-format on
};

} // namespace bee
