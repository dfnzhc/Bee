/**
 * @File RenderDevice.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#include "Render/RenderDevice.hpp"

using namespace bee;

Error RenderDevice::init(const RenderDeviceConfig& config)
{
    _context = CreateRenderContext(config.deviceType);
    _context->initialize();

    _driver = _context->createDriver();
    _driver->initialize(0, 4);

    return Error::Ok;
}

void RenderDevice::shutdown()
{
    _driver.reset();
    _context.reset();
}
