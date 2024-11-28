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

namespace bee {

class RenderDriver;

class BEE_API RenderContext
{
public:
    enum class DeviceType
    {
        Unknown = 0,
        IntegratedGPU,
        DiscreteGPU,
        CPU,
    };

    struct DeviceInfo
    {
        String name     = "Unknown";
        DeviceType type = DeviceType::Unknown;
    };

    virtual ~RenderContext() = default;

    // clang-format off
    virtual Error initialize() = 0;

    BEE_NODISCARD virtual const DeviceInfo& deviceInfo(u32 devIdx) const = 0;
    BEE_NODISCARD virtual u32 deviceCount() const                        = 0;
    BEE_NODISCARD virtual bool deviceSupportsPresent(u32 devIdx) const   = 0;

    BEE_NODISCARD virtual std::unique_ptr<RenderDriver> createDriver() = 0;
    // clang-format on
};

} // namespace bee
