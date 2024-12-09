/**
 * @File Application.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>
#include "Base/Window.hpp"

#include "Render/RenderCommon.hpp"

namespace bee {
struct AppConfig
{
    Window::Desc windowDesc;
    std::string_view appName;

    bool headless = false;
};

class RenderDevice;

class BEE_API Application : public Window::ICallbacks
{
public:
    explicit Application(const AppConfig& config);
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

    virtual RenderDeviceConfig createDevices() const;

private:
    void handleWindowSizeChange(u32 width, u32 height) override;
    void handleKeyboardEvent(const KeyboardEvent& keyEvent) override;
    void handleMouseEvent(const MouseEvent& mouseEvent) override;

    void handleMouseKeyboardEvent(const MouseInput& mouse, const KeyboardInput& keyboard);

private:
    AppConfig _config;
    UniquePtr<Window> _pWindow;

    // TODO: 支持多个渲染设备？
    UniquePtr<RenderDevice> _pRenderDevice;

    InputManager _inputManager;
    bool _shouldTerminate = false;
};
} // namespace bee