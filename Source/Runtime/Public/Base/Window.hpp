/**
 * @File Window.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Math/Math.hpp"
#include "Platform/Handles.hpp"
#include <functional>

namespace bee {

class BEE_API Window
{
public:
    using Handle    = WindowHandle;
    using ApiHandle = WindowApiHandle;

    enum class Mode
    {
        Normal,
        Fullscreen,
        Minimized,
    };

    struct Desc
    {
        vec2u extent      = {1'920, 1'080};
        std::string title = "Bee Engine";
        Mode mode         = Mode::Normal;

        bool resizable   = true;
        bool enableVSync = false;
    };

    class ICallbacks
    {
    public:
        virtual void handleWindowSizeChange() = 0;
    };

    Window(const Desc& desc, ICallbacks* pCallbacks);
    ~Window();

    // clang-format off
    void shutdown();
    BEE_NODISCARD bool shouldClose() const;

    void resize(u32 width, u32 height);
    void resize(vec2u extent) { resize(extent.x, extent.y); }

    void pollForEvents();

    void setPos(int x, int y);
    void setPos(vec2i pos) { setPos(pos.x, pos.y); }

    void setTitle(std::string title);
    void setIcon(const std::filesystem::path& path);

    BEE_NODISCARD vec2u extent() const { return _desc.extent; }
    BEE_NODISCARD   u32 width()  const { return _desc.extent.x; }
    BEE_NODISCARD   u32 height() const { return _desc.extent.y; }
    
    BEE_NODISCARD const Handle& handle() const { return _handle; }
    BEE_NODISCARD const   Desc& desc()   const { return _desc; }
    // clang-format on

private:
    void setWindowSize(u32 width, u32 height);
    void updateWindowSize();
    
private:
    Desc _desc           = {};
    Handle _handle       = {};
    ApiHandle _apiHandle = {};

    ICallbacks* _pCallbacks = nullptr;
};

} // namespace bee