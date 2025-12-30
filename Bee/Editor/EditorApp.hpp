/**
 * @File Editor.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/2
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Bee/Application/BaseApp.hpp"

namespace bee
{
class EditorApp : public BaseApp
{
public:
    EditorApp();
    ~EditorApp() override;

protected:
    bool OnInit() override;
    void OnUpdate(double dt) override;
    void OnRender() override;
    void OnQuit() override;
    bool OnEvent() override;

private:

};

BEE_CREATE_APP(EditorApp)

} // namespace bee
