/**
 * @File GFX_Device.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#include "GFX/GFX_Device.hpp"

using namespace bee;

Error GFX_Device::create(const RenderDeviceConfig& config)
{
    _context = CreateRenderContext(config.deviceType);
    GFX_Context::Config ctxConfig{};
    ctxConfig.headless = config.headless;
    BEE_REPORT_IF_FAILED(_context->create(ctxConfig));

    _driver = _context->createDriver();
    BEE_REPORT_IF_FAILED(_driver->create(0, 4));

    return Error::Ok;
}

void GFX_Device::destroy()
{
    if (_driver)
        _driver->destroy();
    
    if (_context)
        _context->destroy();
}
