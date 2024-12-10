/**
 * @File Window.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#include "Base/Window.hpp"
#include <Utility/Logger.hpp>
//#include <Platform/Platform.hpp>

using namespace bee;

class bee::ApiCallbacks
{
public:
    static void ErrorCallback(int errorCode, const char* pDescription) { LogError("GLFW 错误{}: '{}'", errorCode, pDescription); }

    static void WindowSizeCallback(GLFWwindow* pGlfwWindow, int width, int height)
    {
        // We also get here in case the window was minimized, so we need to ignore it
        if (width == 0 || height == 0) {
            return;
        }

        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            pWindow->resize(width, height); // Window callback is handled in here
        }
    }

    static void KeyboardCallback(GLFWwindow* pGlfwWindow, int key, int /*scanCode*/, int action, int modifiers)
    {
        KeyboardEvent event;
        if (PrepareKeyboardEvent(key, action, modifiers, event)) {
            auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
            if (pWindow != nullptr) {
                pWindow->_pCallbacks->handleKeyboardEvent(event);
            }
        }
    }

    static void CharInputCallback(GLFWwindow* pGlfwWindow, u32 input)
    {
        KeyboardEvent event;
        event.type = KeyboardEvent::Type::Input;

        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            pWindow->_pCallbacks->handleKeyboardEvent(event);
        }
    }

    static void MouseButtonCallback(GLFWwindow* pGlfwWindow, int button, int action, int modifiers)
    {
        MouseEvent event;
        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT   : event.button = MouseEvent::Button::Left; break;
        case GLFW_MOUSE_BUTTON_MIDDLE : event.button = MouseEvent::Button::Middle; break;
        case GLFW_MOUSE_BUTTON_RIGHT  : event.button = MouseEvent::Button::Right; break;
        default                       : return;
        }

        event.type = (action == GLFW_PRESS) ? MouseEvent::Type::ButtonDown : MouseEvent::Type::ButtonUp;

        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            // Modifiers
            double x, y;
            glfwGetCursorPos(pGlfwWindow, &x, &y);
            event.pos = CalcMousePos(x, y, pWindow->_getMouseScale());

            pWindow->_pCallbacks->handleMouseEvent(event);
        }
    }

    static void MouseMoveCallback(GLFWwindow* pGlfwWindow, double mouseX, double mouseY)
    {
        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            MouseEvent event;
            event.type       = MouseEvent::Type::Move;
            event.pos        = CalcMousePos(mouseX, mouseY, pWindow->_getMouseScale());
            event.screenPos  = {mouseX, mouseY};
            event.wheelDelta = vec2f(0, 0);

            pWindow->_pCallbacks->handleMouseEvent(event);
        }
    }

    static void MouseWheelCallback(GLFWwindow* pGlfwWindow, double scrollX, double scrollY)
    {
        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            MouseEvent event;
            event.type = MouseEvent::Type::Wheel;
            double x, y;
            glfwGetCursorPos(pGlfwWindow, &x, &y);
            event.pos        = CalcMousePos(x, y, pWindow->_getMouseScale());
            event.wheelDelta = (vec2f(float(scrollX), float(scrollY)));

            pWindow->_pCallbacks->handleMouseEvent(event);
        }
    }

private:
    inline static KeyboardEvent::Key GLFWToBeeKey(int glfwKey)
    {
        static_assert(GLFW_KEY_ESCAPE == 256, "GLFW_KEY_ESCAPE 需要是 256");
        static_assert((u32)KeyboardEvent::Key::Escape >= 256, "Key::Escape 至少需要是 256");

        if (glfwKey < GLFW_KEY_ESCAPE) {
            // 与定义相同，直接返回
            return (KeyboardEvent::Key)glfwKey;
        }

        switch (glfwKey) {
        case GLFW_KEY_ESCAPE        : return KeyboardEvent::Key::Escape;
        case GLFW_KEY_ENTER         : return KeyboardEvent::Key::Enter;
        case GLFW_KEY_TAB           : return KeyboardEvent::Key::Tab;
        case GLFW_KEY_BACKSPACE     : return KeyboardEvent::Key::Backspace;
        case GLFW_KEY_INSERT        : return KeyboardEvent::Key::Insert;
        case GLFW_KEY_DELETE        : return KeyboardEvent::Key::Del;
        case GLFW_KEY_RIGHT         : return KeyboardEvent::Key::Right;
        case GLFW_KEY_LEFT          : return KeyboardEvent::Key::Left;
        case GLFW_KEY_DOWN          : return KeyboardEvent::Key::Down;
        case GLFW_KEY_UP            : return KeyboardEvent::Key::Up;
        case GLFW_KEY_PAGE_UP       : return KeyboardEvent::Key::PageUp;
        case GLFW_KEY_PAGE_DOWN     : return KeyboardEvent::Key::PageDown;
        case GLFW_KEY_HOME          : return KeyboardEvent::Key::Home;
        case GLFW_KEY_END           : return KeyboardEvent::Key::End;
        case GLFW_KEY_CAPS_LOCK     : return KeyboardEvent::Key::CapsLock;
        case GLFW_KEY_SCROLL_LOCK   : return KeyboardEvent::Key::ScrollLock;
        case GLFW_KEY_NUM_LOCK      : return KeyboardEvent::Key::NumLock;
        case GLFW_KEY_PRINT_SCREEN  : return KeyboardEvent::Key::PrintScreen;
        case GLFW_KEY_PAUSE         : return KeyboardEvent::Key::Pause;
        case GLFW_KEY_F1            : return KeyboardEvent::Key::F1;
        case GLFW_KEY_F2            : return KeyboardEvent::Key::F2;
        case GLFW_KEY_F3            : return KeyboardEvent::Key::F3;
        case GLFW_KEY_F4            : return KeyboardEvent::Key::F4;
        case GLFW_KEY_F5            : return KeyboardEvent::Key::F5;
        case GLFW_KEY_F6            : return KeyboardEvent::Key::F6;
        case GLFW_KEY_F7            : return KeyboardEvent::Key::F7;
        case GLFW_KEY_F8            : return KeyboardEvent::Key::F8;
        case GLFW_KEY_F9            : return KeyboardEvent::Key::F9;
        case GLFW_KEY_F10           : return KeyboardEvent::Key::F10;
        case GLFW_KEY_F11           : return KeyboardEvent::Key::F11;
        case GLFW_KEY_F12           : return KeyboardEvent::Key::F12;
        case GLFW_KEY_KP_0          : return KeyboardEvent::Key::Keypad0;
        case GLFW_KEY_KP_1          : return KeyboardEvent::Key::Keypad1;
        case GLFW_KEY_KP_2          : return KeyboardEvent::Key::Keypad2;
        case GLFW_KEY_KP_3          : return KeyboardEvent::Key::Keypad3;
        case GLFW_KEY_KP_4          : return KeyboardEvent::Key::Keypad4;
        case GLFW_KEY_KP_5          : return KeyboardEvent::Key::Keypad5;
        case GLFW_KEY_KP_6          : return KeyboardEvent::Key::Keypad6;
        case GLFW_KEY_KP_7          : return KeyboardEvent::Key::Keypad7;
        case GLFW_KEY_KP_8          : return KeyboardEvent::Key::Keypad8;
        case GLFW_KEY_KP_9          : return KeyboardEvent::Key::Keypad9;
        case GLFW_KEY_KP_DECIMAL    : return KeyboardEvent::Key::KeypadDel;
        case GLFW_KEY_KP_DIVIDE     : return KeyboardEvent::Key::KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY   : return KeyboardEvent::Key::KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT   : return KeyboardEvent::Key::KeypadSubtract;
        case GLFW_KEY_KP_ADD        : return KeyboardEvent::Key::KeypadAdd;
        case GLFW_KEY_KP_ENTER      : return KeyboardEvent::Key::KeypadEnter;
        case GLFW_KEY_KP_EQUAL      : return KeyboardEvent::Key::KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT    : return KeyboardEvent::Key::LeftShift;
        case GLFW_KEY_LEFT_CONTROL  : return KeyboardEvent::Key::LeftControl;
        case GLFW_KEY_LEFT_ALT      : return KeyboardEvent::Key::LeftAlt;
        case GLFW_KEY_LEFT_SUPER    : return KeyboardEvent::Key::LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT   : return KeyboardEvent::Key::RightShift;
        case GLFW_KEY_RIGHT_CONTROL : return KeyboardEvent::Key::RightControl;
        case GLFW_KEY_RIGHT_ALT     : return KeyboardEvent::Key::RightAlt;
        case GLFW_KEY_RIGHT_SUPER   : return KeyboardEvent::Key::RightSuper;
        case GLFW_KEY_MENU          : return KeyboardEvent::Key::Menu;
        default                     : return KeyboardEvent::Key::Unknown;
        }
    }

    inline static KeyboardEvent::ModifierFlags GetModifierFlags(int modifiers)
    {
        KeyboardEvent::ModifierFlags flags = KeyboardEvent::ModifierFlags::None;
        BEE_USE_MAGIC_ENUM_BIT_OPERATOR;
        if (modifiers & GLFW_MOD_ALT)
            flags |= KeyboardEvent::ModifierFlags::Alt;
        if (modifiers & GLFW_MOD_CONTROL)
            flags |= KeyboardEvent::ModifierFlags::Ctrl;
        if (modifiers & GLFW_MOD_SHIFT)
            flags |= KeyboardEvent::ModifierFlags::Shift;

        if (static_cast<int>(flags) == 6) {
            int a = 0;
        }

        return flags;
    }

    int static FixGLFWModifiers(int modifiers, int key, int action)
    {
        int bit = 0;
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
            bit = GLFW_MOD_SHIFT;
        if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
            bit = GLFW_MOD_CONTROL;
        if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
            bit = GLFW_MOD_ALT;
        return (action == GLFW_RELEASE) ? modifiers & (~bit) : modifiers | bit;
    }

    inline static vec2 CalcMousePos(double xPos, double yPos, vec2 mouseScale)
    {
        auto pos  = vec2(float(xPos), float(yPos));
        pos      *= mouseScale;
        return pos;
    }

    inline static bool PrepareKeyboardEvent(int key, int action, int modifiers, KeyboardEvent& event)
    {
        if (key == GLFW_KEY_UNKNOWN) {
            return false;
        }

        // modifiers = FixGLFWModifiers(modifiers, key, action);

        switch (action) {
        case GLFW_RELEASE : event.type = KeyboardEvent::Type::Released; break;
        case GLFW_PRESS   : event.type = KeyboardEvent::Type::Pressed; break;
        case GLFW_REPEAT  : event.type = KeyboardEvent::Type::Repeated; break;
        default           : BEE_UNREACHABLE();
        }
        event.key = GLFWToBeeKey(key);
        // event.mods = GetModifierFlags(modifiers);

        return true;
    }
};

namespace {

std::atomic<u16> sWindowCount = {0};

} // namespace

Window::Window(const Window::Desc& desc, Window::ICallbacks* pCallbacks)
: _desc(desc), _pCallbacks(pCallbacks), _mouseScale(1.0f / (float)desc.extent.x, 1.0f / (float)desc.extent.y)
{
    LogInfo("创建窗口 '{}'({}x{})", desc.title, desc.extent.x, desc.extent.y);

    if (sWindowCount.fetch_add(1) == 0) {
        BEE_ASSERT(glfwInit() == GLFW_TRUE, "GLFW 初始化失败");
    }

    glfwSetErrorCallback(ApiCallbacks::ErrorCallback);
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

    setPos(_desc.pos);
    _updateWindowSize();
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
    glfwSetWindowSizeCallback(_apiHandle, ApiCallbacks::WindowSizeCallback);

    glfwSetKeyCallback(_apiHandle, ApiCallbacks::KeyboardCallback);
    glfwSetCharCallback(_apiHandle, ApiCallbacks::CharInputCallback);

    glfwSetMouseButtonCallback(_apiHandle, ApiCallbacks::MouseButtonCallback);
    glfwSetCursorPosCallback(_apiHandle, ApiCallbacks::MouseMoveCallback);
    glfwSetScrollCallback(_apiHandle, ApiCallbacks::MouseWheelCallback);
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
        _setWindowSize(width, height);
    }
    else {
        _updateWindowSize();
    }

    if (_pCallbacks)
        _pCallbacks->handleWindowSizeChange(width, height);
}

void Window::pollForEvents()
{
    glfwPollEvents();
}

void Window::setPos(int x, int y)
{
    glfwSetWindowPos(_apiHandle, x, y);
}

void Window::setTitle(std::string_view title)
{
    glfwSetWindowTitle(_apiHandle, title.data());
}

void Window::setIcon(const std::filesystem::path& path)
{
    //    SetWindowIcon(path, _handle);
}

void Window::_updateWindowSize()
{
    int width, height;
    glfwGetWindowSize(_apiHandle, &width, &height);
    _setWindowSize((u32)width, (u32)height);

    _mouseScale.x = 1.0f / (float)width;
    _mouseScale.y = 1.0f / (float)height;
}

void Window::_setWindowSize(u32 width, u32 height)
{
    BEE_CHECK(width > 0 && height > 0, "有效的窗口大小应该大于零");

    _desc.extent.x = width;
    _desc.extent.y = height;
}
