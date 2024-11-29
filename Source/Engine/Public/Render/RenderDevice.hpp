/**
 * @File RenderDevice.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Render/RenderContext.hpp"
#include "Render/RenderDriver.hpp"
#include "Render/RenderCommon.hpp"
#include <Utility/Macros.hpp>

namespace bee {

class BEE_API RenderDevice final
{
public:
    RenderDevice() = default;
    ~RenderDevice() = default;
    
    BEE_CLASS_DELETE_COPY(RenderDevice);
    
    Error create(const RenderDeviceConfig& config);
    void destroy();
    
private:
    std::unique_ptr<RenderContext> _context = nullptr;
    std::unique_ptr<RenderDriver> _driver   = nullptr;
};

} // namespace bee