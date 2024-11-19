/**
 * @File Window.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#include "Base/Window.hpp"
#include "Utility/Logger.hpp"
#include "Platform/Platform.hpp"

using namespace bee;

namespace {

void ErrorCallback(int errorCode, const char* pDescription)
{
    LogError("GLFW 错误{}: '{}'", errorCode, pDescription);
}

void WindowSizeCallback(GLFWwindow* pGlfwWindow, int width, int height)
{
    // We also get here in case the window was minimized, so we need to ignore it
    if (width == 0 || height == 0) {
        return;
    }

    Window* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
    if (pWindow != nullptr) {
        pWindow->resize(width, height); // Window callback is handled in here
    }
}

std::atomic<u16> sWindowCount = {0};

} // namespace

Window::Window(const Window::Desc& desc, Window::ICallbacks* pCallbacks) : _desc(desc), _pCallbacks(pCallbacks)
{
    LogInfo("创建窗口 '{}'({}x{})", desc.title, desc.extent.x, desc.extent.y);

    glfwSetErrorCallback(ErrorCallback);

    if (sWindowCount.fetch_add(1) == 0) {
        BEE_ASSERT(glfwInit() == GLFW_TRUE, "GLFW 初始化失败");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (!desc.resizable) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        LogInfo("\t-- 不可缩放.");
    }

    vec2i extent = {(int)desc.extent.x, (int)desc.extent.y};

    if (desc.mode == Window::Mode::Fullscreen) {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        auto mon = glfwGetPrimaryMonitor();
        auto mod = glfwGetVideoMode(mon);
        extent   = {mod->width, mod->height};
    }
    else if (desc.mode == Window::Mode::Minimized) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
    }

    _apiHandle = glfwCreateWindow(extent.x, extent.y, desc.title.c_str(), nullptr, nullptr);
    BEE_ASSERT(_apiHandle, "创建 GLFW 窗口失败");

#ifdef BEE_IN_WINDOWS
    _handle = glfwGetWin32Window(_apiHandle);
    BEE_ASSERT(_handle, "获取 Win32 窗口 Handle 失败");
#endif

    updateWindowSize();
    glfwSetWindowUserPointer(_apiHandle, this);

    if (desc.mode == Window::Mode::Minimized) {
        glfwIconifyWindow(_apiHandle);
        glfwShowWindow(_apiHandle);
    }
    else {
        glfwShowWindow(_apiHandle);
        glfwFocusWindow(_apiHandle);
    }

    // 设置回调
    glfwSetWindowSizeCallback(_apiHandle, WindowSizeCallback);
}

Window::~Window()
{
    glfwDestroyWindow(_apiHandle);

    if (sWindowCount.fetch_sub(1) == 1)
        glfwTerminate();
}

void Window::shutdown()
{
    glfwSetWindowShouldClose(_apiHandle, 1);
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(_apiHandle);
}

void Window::resize(u32 width, u32 height)
{
    glfwSetWindowSize(_apiHandle, (int)width, (int)height);

    // In minimized mode GLFW reports incorrect window size
    if (_desc.mode == Window::Mode::Minimized) {
        setWindowSize(width, height);
    }
    else {
        updateWindowSize();
    }

    if (_pCallbacks)
        _pCallbacks->handleWindowSizeChange();
}

void Window::pollForEvents()
{
    glfwPollEvents();
}

void Window::setPos(int x, int y)
{
    glfwSetWindowPos(_apiHandle, x, y);
}

void Window::setTitle(std::string title)
{
    glfwSetWindowTitle(_apiHandle, title.c_str());
}

void Window::setIcon(const std::filesystem::path& path)
{
    SetWindowIcon(path, _handle);
}

void Window::updateWindowSize()
{
    int width, height;
    glfwGetWindowSize(_apiHandle, &width, &height);
    setWindowSize((u32)width, (u32)height);
}

void Window::setWindowSize(u32 width, u32 height)
{
    BEE_ASSUME(width > 0 && height > 0);

    _desc.extent.x = width;
    _desc.extent.y = height;
}
