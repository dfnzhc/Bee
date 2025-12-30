/**
 * @File BaseApp.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Bee/Platform/IApplication.hpp"

namespace bee
{

class BaseApp : public IApplication
{
public:
    BaseApp();
    ~BaseApp() override;

    BaseApp(const BaseApp&)            = delete;
    BaseApp& operator=(const BaseApp&) = delete;

    bool initialize(int argc, char* argv[]) final;
    void runFrame() final;
    void processEvent() final;

    void shutdown() final;
    void requestQuit() final;
    bool shouldQuit() const final;

protected:
    // -------------------- 
    // 在子类中实现
    
    virtual bool OnInit();
    virtual void OnUpdate(double dt);
    virtual void OnRender();
    virtual void OnQuit();

    virtual bool OnEvent();

private:
    bool _shouldQuit = false;
};

} // namespace bee
