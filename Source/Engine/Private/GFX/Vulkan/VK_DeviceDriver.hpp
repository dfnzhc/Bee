/**
 * @File VK_DeviceDriver.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "GFX/GFX_DeviceDriver.hpp"

namespace bee {

class VK_Context;

class BEE_API VK_DeviceDriver final : public GFX_DeviceDriver
{
public:
    VK_DeviceDriver(VK_Context* context);
    
    ~VK_DeviceDriver() override;
    Error create(u32 deviceIndex, u32 frameCount) override;
    void destroy() override;
    
private:
    VK_Context* _context = nullptr;
};

} // namespace bee