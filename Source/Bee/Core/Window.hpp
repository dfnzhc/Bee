/**
 * @File Window.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"
#include "Base/Error.hpp"
#include "Base/Memory.hpp"
#include "Base/Macros.hpp"
#include "Base/Object.hpp"
#include "Base/Thirdparty.hpp"

#include "Math/Math.hpp"

#include <SDL3/SDL_events.h>

class SDL_Window;
class SDL_Renderer;

namespace bee {
class BEE_API Window : public NonCopyable
{
public:
    Window();
    ~Window() override;

    // clang-format off

    /// ==========================
    /// behaviours
    /// ==========================

    void initialize();
    void shutdown();
    void pollForEvents();

    void show() const;
    void hide() const;

    void resize(int width, int height);
    void resize(vec2i extent) { resize(extent.x, extent.y); }

    void setPos(int posX, int posY) const;
    void setPos(vec2i pos) const { setPos(pos.x, pos.y); }

    void setTitle(StringView title) const;
    void setIcon(StringView path) const;
    void setVSync(bool enabled) const;
    
    // window properties
    BEE_NODISCARD vec2u extent()     const;
    BEE_NODISCARD u32   width()      const;
    BEE_NODISCARD u32   height()     const;
    BEE_NODISCARD f32   dpiScale()   const;
    
    BEE_NODISCARD Result<vec2i> pos() const;

    BEE_NODISCARD bool isRequestExit() const;
    
    BEE_NODISCARD VoidPtr         handleSDL() const;
    BEE_NODISCARD Result<VoidPtr> handleRaw() const;
    
    BEE_NODISCARD VoidPtr         rendererSDL() const;
    // clang-format on

    vec2i clampPos(int xPos, int yPos) const;
    vec2i clampPos(vec2i pos) const { return clampPos(pos.x, pos.y); }

    vec2f clampPosNormalized(int xPos, int yPos) const;
    vec2f clampPosNormalized(vec2i pos) const { return clampPosNormalized(pos.x, pos.y); }

private:
    SDL_Window* _pWindow     = nullptr;
    SDL_Renderer* _pRenderer = nullptr; ///< Default renderer

    vec2f _mouseScale = {};
    int _width        = 1920, _height = 1080;

    bool _isRequestExit = false;
};
} // namespace bee