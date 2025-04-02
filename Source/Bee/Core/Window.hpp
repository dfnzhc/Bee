/**
 * @File Window.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"
#include "Base/Macros.hpp"
#include "Base/Thirdparty.hpp"

#include "Math/Math.hpp"

namespace bee {
class BEE_API IWindowCallbacks
{
public:
    virtual ~IWindowCallbacks() = default;

    virtual void onWindowSizeChanged(int width, int height) = 0;

    // ...
};

class BEE_API Window
{
public:
    Window(IWindowCallbacks* pCallbacks);
    ~Window();

    BEE_CLASS_DELETE_COPY(Window);

    // clang-format off

    /// ==========================
    /// behaviours
    /// ==========================

    void initialize();
    void shutdown();
    void pollForEvents();

    void show();
    void hide();
    void focus();
    
    void fullScreen();
    void windowed();
    void fullScreenBorderless();
    
    void minimize();
    void maximize();

    void resize(int width, int height);
    void resize(vec2i extent) { resize(extent.x, extent.y); }

    void setPos(int posX, int posY);
    void setPos(vec2i pos) { setPos(pos.x, pos.y); }

    void setTitle(StringView title);
    void setIcon(StringView path);
    
    // window properties
    BEE_NODISCARD vec2u extent()   const;
    BEE_NODISCARD u32   width()    const;
    BEE_NODISCARD u32   height()   const;
    BEE_NODISCARD vec2u pos()      const;
    BEE_NODISCARD f32   dpiScale() const;

    BEE_NODISCARD bool IsMinimized()   const;
    BEE_NODISCARD bool IsFullScreen()  const;
    BEE_NODISCARD bool isRequestExit() const;
    
    BEE_NODISCARD void* handleSDL() const;
    BEE_NODISCARD void* handleRaw() const;
    // clang-format on
};
} // namespace bee