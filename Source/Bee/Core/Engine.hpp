/**
 * @File Engine.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/1
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"
#include "Base/Macros.hpp"
#include "Base/Object.hpp"
#include "Base/Memory.hpp"

namespace bee {
class Gui;
class Window;

class BEE_API Engine final : public NonCopyable
{
public:
    Engine();
    ~Engine() override;

    /// ==========================
    /// behaviours
    /// ==========================
    int execute();

private:
    /// ==========================
    /// events handle
    /// ==========================
    void _onWindowSizeChanged(int width, int height);

private:
    bool _initialize();
    void _shutdown();

    void _initializeWindow();
    void _shutdownWindow();

    void _initializeGui();
    void _shutdownGui();

    void _update();

private:
    UniquePtr<Window> _pWindow;
    UniquePtr<Gui> _pGui;
};
} // namespace bee