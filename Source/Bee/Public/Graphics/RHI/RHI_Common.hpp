/**
 * @File RHI_Common.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Utility/Enum.hpp"
#include "Platform/Platform.hpp"

namespace bee {

enum class Vendor : u16
{
    Unknown   = 0x0,
    AMD       = 0x1002,
    ImgTec    = 0x1010,
    Apple     = 0x106B,
    Nvidia    = 0x10DE,
    ARM       = 0x13B5,
    Microsoft = 0x1414,
    Qualcomm  = 0x5143,
    INTEL     = 0x8086
};

enum class DeviceType : u8
{
    Other         = 0x0,
    IntegratedGPU = 0x1,
    DiscreteGPU   = 0x2,
    VirtualGPU    = 0x3,
    CPU           = 0x4
};

enum class GraphicsAPI : u8
{
    Unknown = 0x0,
    Vulkan  = 0x1,
    D3D12   = 0x2,
};
} // namespace bee