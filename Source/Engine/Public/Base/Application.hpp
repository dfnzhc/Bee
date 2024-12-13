/**
 * @File Application.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>
#include "Base/Window.hpp"

namespace bee {

struct AppSettings
{
    Window::Desc windowDesc;
    std::string_view appName;

    bool headless = false;
};

class GFX_Device;

class BEE_API Application : public Window::ICallbacks
{
public:
    explicit Application(const AppSettings& config);
    ~Application() override;

    BEE_CLASS_DELETE_MOVE(Application);
    BEE_CLASS_DELETE_COPY(Application);

    int preInit();
    int init();

    void tick();
    void shutdown();

    BEE_NODISCARD Window* window() const;
    BEE_NODISCARD bool shouldTerminate() const;

protected:
    virtual int onInit();
    virtual void onTick();
    virtual void onShutdown();
    virtual void onResize(u32 width, u32 height);
    virtual bool onMouseEvent(const MouseEvent& mouseEvent);
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent);

    virtual bool onMouseKeyboardEvent(const MouseInput& mouse, const KeyboardInput& keyboard);

private:
    void handleWindowSizeChange(u32 width, u32 height) override;
    void handleKeyboardEvent(const KeyboardEvent& keyEvent) override;
    void handleMouseEvent(const MouseEvent& mouseEvent) override;

    void handleMouseKeyboardEvent(const MouseInput& mouse, const KeyboardInput& keyboard);

private:
    AppSettings _config;
    UniquePtr<Window> _pWindow;

    InputManager _inputManager;
    bool _shouldTerminate = false;
};
} // namespace bee