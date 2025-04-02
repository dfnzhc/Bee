/**
 * @File Window.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */

#include "Core/Window.hpp"

using namespace bee;

Window::Window(IWindowCallbacks* pCallbacks)
{
}

Window::~Window()
{
}

void Window::initialize()
{
}

void Window::shutdown()
{
}

void Window::pollForEvents()
{
}

void Window::show()
{
}

void Window::hide()
{
}

void Window::focus()
{
}

void Window::fullScreen()
{
}

void Window::windowed()
{
}

void Window::fullScreenBorderless()
{
}

void Window::minimize()
{
}

void Window::maximize()
{
}

void Window::resize(int width, int height)
{
}

void Window::setPos(int posX, int posY)
{
}

void Window::setTitle(StringView title)
{
}

void Window::setIcon(StringView path)
{
}

vec2u Window::extent() const
{
    return {};
}

u32 Window::width() const
{
    return {};
}

u32 Window::height() const
{
    return {};
}

vec2u Window::pos() const
{
    return {};
}

f32 Window::dpiScale() const
{
    return {};
}

bool Window::IsMinimized() const
{
    return {};
}

bool Window::IsFullScreen() const
{
    return {};
}

bool Window::isRequestExit() const
{
    return {};
}

void* Window::handleSDL() const
{
    return {};
}

void* Window::handleRaw() const
{
    return {};
}