/**
 * @File Engine.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/1
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"
#include "Base/Macros.hpp"

#include "Core/Window.hpp"

namespace bee {

class Gui;

class BEE_API Engine final : public IWindowCallbacks, public NonCopyable
{
public:
    Engine();
    ~Engine() override;

    /// ==========================
    /// behaviours
    /// ==========================
    int execute();

    /// ==========================
    /// window callbacks
    /// ==========================
    void onWindowSizeChanged(int width, int height) override;

private:

    bool _initialize();
    void _shutdown();

    void _initializeWindow();
    void _shutdownWindow();

    void _initializeGui();
    void _shutdownGui();

    void _update();
    void _renderFrame();

private:
    UniquePtr<Window> _pWindow = nullptr;
    UniquePtr<Gui> _pGui       = nullptr;
};

} // namespace bee