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

class BEE_API Engine final : public IWindowCallbacks
{
public:
    Engine();
    ~Engine() override;

    BEE_CLASS_DELETE_COPY(Engine);

    /// ==========================
    /// behaviours
    /// ==========================
    int execute();

    
    /// ==========================
    /// window callbacks
    /// ==========================
    void onWindowSizeChanged(int width, int height) override;
};

} // namespace bee