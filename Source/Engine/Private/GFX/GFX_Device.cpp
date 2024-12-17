/**
 * @File GFX_Device.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#include "GFX/GFX_Device.hpp"

using namespace bee;

Error GFX_Device::create(const GFX_Desc& desc)
{
    GFX_Context::Config ctxConfig{};
    ctxConfig.headless = false;

    _context = CreateRenderContext(desc.gfxApi);
    BEE_REPORT_IF_FAILED(_context->create(ctxConfig));

    // TODO: 根据上下文信息选择更好的设备
    GFX_DeviceDriver::Config driverConfig{};
    driverConfig.deviceIndex = 0;
    driverConfig.frameCount  = 3;
    driverConfig.headless    = ctxConfig.headless;
    driverConfig.raytracing  = false;

    _driver = _context->createDriver();
    BEE_REPORT_IF_FAILED(_driver->create(driverConfig));

    return Error::Ok;
}

void GFX_Device::destroy()
{
    if (_driver)
        _driver->destroy();

    if (_context)
        _context->destroy();
}
