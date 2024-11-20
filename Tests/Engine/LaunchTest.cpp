/**
 * @File EngineTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/15
 * @Brief This file is part of Bee.
 */

#include <Launch/EntryPoint.hpp>

using namespace bee;

class TestApp : public Application
{
public:
    explicit TestApp(const AppConfig& config) : Application(config) { }

    ~TestApp() override { }

protected:
    int onInit() override { return Application::onInit(); }

    void onTick() override { Application::onTick(); }

    void onShutdown() override { Application::onShutdown(); }

    void onResize(u32 width, u32 height) override { Application::onResize(width, height); }

    bool onMouseEvent(const MouseEvent& mouseEvent) override { return Application::onMouseEvent(mouseEvent); }

    bool onKeyEvent(const KeyboardEvent& keyEvent) override { return Application::onKeyEvent(keyEvent); }
};

BeeLaunchParam bee::LaunchParamSetup(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nCmdShow)
{
    BeeLaunchParam param;
    param.createFunc = [] {
        AppConfig config;
        config.windowDesc.title = "Launch Test";
        config.appName          = "Launch Test App";

        return std::make_unique<TestApp>(config);
    };
    
    return param;
}
