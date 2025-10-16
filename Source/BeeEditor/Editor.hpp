/**
 * @File Editor.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Application.hpp"

namespace Bee
{
    class Editor final : public Application
    {
    public:
        Editor();
        ~Editor() override;

    protected:
        bool onInitialize() override;
        void onShutdown() override;
        bool onPrepareRun() override;
        void onFinishRun() override;
    };
} // namespace Bee
