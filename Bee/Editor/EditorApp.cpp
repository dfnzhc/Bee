/**
 * @File Editor.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/2
 * @Brief This file is part of Bee.
 */

#include "EditorApp.hpp"
#include <Bee/Bee.hpp>

using namespace bee;

EditorApp::EditorApp()
{
}

EditorApp::~EditorApp()
{
}

bool EditorApp::OnInit()
{
    BEE_INFO("EditorApp onInit");

    return true;
}

void EditorApp::OnUpdate(double dt)
{
    BEE_INFO("EditorApp onUpdate");
}

void EditorApp::OnRender()
{
    BEE_INFO("EditorApp onRender");
}

void EditorApp::OnQuit()
{
    BEE_INFO("EditorApp onQuit");
}

bool EditorApp::OnEvent()
{
    return true;
}
