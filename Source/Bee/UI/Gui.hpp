/**
 * @File Gui.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/6
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

namespace bee {

class BEE_API Gui
{
public:
    Gui(void* pWindowSDL, void* pRendererSDL, f32 dpiScale);
    ~Gui();

    void beginFrame() const;
    void present() const;

    bool handleEvents(void* pEventSDL) const;

private:
};
} // namespace bee