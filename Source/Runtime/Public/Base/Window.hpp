/**
 * @File Window.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Utility/Macros.hpp"
#include "Math/Math.hpp"
#include "Platform/Handles.hpp"
#include "IO/Inputs.hpp"
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
        vec2u pos         = {50, 100};
        std::string title = "Bee Engine";
        Mode mode         = Mode::Normal;

        bool resizable   = true;
        bool enableVSync = false;
    };

    class ICallbacks
    {
    public:
        virtual ~ICallbacks() = default;

        virtual void handleWindowSizeChange(u32 width, u32 height) = 0;

        virtual void handleKeyboardEvent(const KeyboardEvent& keyEvent) = 0;
        virtual void handleMouseEvent(const MouseEvent& mouseEvent)     = 0;
    };

    Window(const Desc& desc, ICallbacks* pCallbacks);
    ~Window();

    BEE_CLASS_DELETE_COPY(Window);

    // clang-format off
    void shutdown();
    BEE_NODISCARD bool shouldClose() const;

    void resize(u32 width, u32 height);
    void resize(vec2u extent) { resize(extent.x, extent.y); }

    void pollForEvents();

    void setPos(int x, int y);
    void setPos(vec2i pos) { setPos(pos.x, pos.y); }

    void setTitle(std::string_view title);
    void setIcon(const std::filesystem::path& path);

    BEE_NODISCARD vec2u extent() const { return _desc.extent; }
    BEE_NODISCARD   u32 width()  const { return _desc.extent.x; }
    BEE_NODISCARD   u32 height() const { return _desc.extent.y; }
    BEE_NODISCARD vec2u pos()    const { return _desc.pos; }
    
    BEE_NODISCARD const Handle& handle() const { return _handle; }
    BEE_NODISCARD const   Desc& desc()   const { return _desc; }

    // clang-format on

private:
    void _setWindowSize(u32 width, u32 height);
    void _updateWindowSize();

    BEE_NODISCARD const vec2& _getMouseScale() const { return _mouseScale; }

    friend class ApiCallbacks;

private:
    Desc _desc              = {};
    Handle _handle          = {};
    ApiHandle _apiHandle    = {};
    ICallbacks* _pCallbacks = nullptr;

    vec2 _mouseScale;
};

} // namespace bee