/**
 * @File GFX_Device.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "GFX/GFX.hpp"
#include "GFX/GFX_Context.hpp"
#include "GFX/GFX_DeviceDriver.hpp"
#include <Memory/Memory.hpp>
#include <Utility/Macros.hpp>

namespace bee {

class BEE_API GFX_Device final
{
public:
    GFX_Device()  = default;
    ~GFX_Device() = default;

    BEE_CLASS_DELETE_COPY(GFX_Device);

    Error create(const GFX_Desc& desc);
    void destroy();

private:
    UniquePtr<GFX_Context> _context     = nullptr;
    UniquePtr<GFX_DeviceDriver> _driver = nullptr;
};

} // namespace bee