/**
 * @File BaseApp.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/30
 * @Brief This file is part of Bee.
 */

#include "BaseApp.hpp"
#include "Bee/Platform/Platform.hpp"

using namespace bee;

BaseApp::BaseApp()
{
}

BaseApp::~BaseApp()
{
}

bool BaseApp::initialize(int argc, char* argv[])
{
    Platform::Initialize();
    BEE_INFO("BaseApp init");

    OnInit();

    return true;
}

void BaseApp::runFrame()
{
    // BEE_INFO("BaseApp runFrame");
    // OnUpdate();
    // OnRender();
}

void BaseApp::processEvent()
{
}

void BaseApp::shutdown()
{
    OnQuit();

    BEE_INFO("BaseApp shutdown.");
    Platform::Shutdown();
}

void BaseApp::requestQuit()
{
    _shouldQuit = true;
    BEE_INFO("BaseApp requestQuit.");
}

bool BaseApp::shouldQuit() const
{
    return _shouldQuit;
}

bool BaseApp::OnInit()
{
    return true;
}

void BaseApp::OnUpdate(double dt)
{
}

void BaseApp::OnRender()
{
}

void BaseApp::OnQuit()
{
}

bool BaseApp::OnEvent()
{
    return true;
}
