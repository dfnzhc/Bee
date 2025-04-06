/**
 * @File Gui.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/6
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"
#include "Base/Memory.hpp"

class SDL_Window;
class SDL_Renderer;

namespace bee {

class BEE_API Gui
{
public:
    Gui(void* pWindowSDL, void* pRendererSDL, f32 dpiScale);
    ~Gui() = default;

    void onWindowResize(int width, int height);

    void render();
private:
    class GuiImpl;
    UniquePtr<GuiImpl> _impl;
};
} // namespace bee