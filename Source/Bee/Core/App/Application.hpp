/**
 * @File Application.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/1
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Concepts/NonCopyable.hpp"
#include "Events/Event.hpp"

namespace bee {
class Gui;

class BEE_API Application final : public NonCopyable
{
public:
    Application();
    ~Application() override;

    /// ==========================
    /// behaviours
    /// ==========================
    int execute();

private:
    void _registerInputEvent();
    
    /// ==========================
    /// events handle
    /// ==========================
    void _onDispatchEvent(const EventPtr& event) const;
    void _onKeyboardEvent(const EventPtr& event);
    void _onMouseEvent(const EventPtr& event);
    void _onWindowSizeChanged(const EventPtr& event);

private:
    bool _initialize();
    void _shutdown();

    void _update();

private:
};
} // namespace bee