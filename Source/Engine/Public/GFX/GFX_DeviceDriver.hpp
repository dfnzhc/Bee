/**
 * @File GFX_DeviceDriver.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/21
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>
#include <Core/Portability.hpp>
#include <Utility/Error.hpp>

namespace bee {

// TODO: 当前只是列出接口

using Handle    = int;
using BufferID  = int;
using TextureID = int;

template<typename T> using Vector = std::vector<T>;

class BEE_API GFX_DeviceDriver
{
public:
    virtual ~GFX_DeviceDriver() = default;

    virtual Error create(u32 deviceIndex, u32 frameCount) = 0;
    virtual void destroy()                                = 0;
};

} // namespace bee