/**
 * @File Editor.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/13
 * @Brief This file is part of Bee.
 */

#include "Editor.hpp"

using namespace Bee;

Editor::Editor() :
    Application("Bee编辑器")
{
}

Editor::~Editor()
{
}

bool Editor::onInitialize()
{
    return Application::onInitialize();
}

void Editor::onShutdown()
{
    Application::onShutdown();
}

bool Editor::onPrepareRun()
{
    return Application::onPrepareRun();
}

void Editor::onFinishRun()
{
    Application::onFinishRun();
}
