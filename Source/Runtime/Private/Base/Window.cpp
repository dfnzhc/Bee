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
        KeyboardEvent event{};
        if (PrepareKeyboardEvent(key, action, modifiers, event)) {
            auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
            if (pWindow != nullptr) {
                pWindow->_pCallbacks->handleKeyboardEvent(event);
            }
        }
    }
    
    static void CharInputCallback(GLFWwindow* pGlfwWindow, u32 input)
    {
        KeyboardEvent event{};
        event.type      = KeyboardEvent::Type::Input;
        event.codepoint = input;

        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            pWindow->_pCallbacks->handleKeyboardEvent(event);
        }
    }

    static void MouseButtonCallback(GLFWwindow* pGlfwWindow, int button, int action, int modifiers)
    {
        MouseEvent event{};
        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT :
            event.button = MouseButton::Left;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE :
            event.button = MouseButton::Middle;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT :
            event.button = MouseButton::Right;
            break;
        default : return;
        }
        
        event.type = (action == GLFW_PRESS) ? MouseEvent::Type::ButtonDown : MouseEvent::Type::ButtonUp;

        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            // Modifiers
            event.mods = GetModifierFlags(modifiers);
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
            MouseEvent event{};
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
            MouseEvent event{};
            event.type = MouseEvent::Type::Wheel;
            double x, y;
            glfwGetCursorPos(pGlfwWindow, &x, &y);
            event.pos        = CalcMousePos(x, y, pWindow->_getMouseScale());
            event.wheelDelta = (vec2f(float(scrollX), float(scrollY)));

            pWindow->_pCallbacks->handleMouseEvent(event);
        }
    }

private:
    inline static Key GLFWToBeeKey(int glfwKey)
    {
        static_assert(GLFW_KEY_ESCAPE == 256, "GLFW_KEY_ESCAPE 需要是 256");
        static_assert((u32)Key::Escape >= 256, "Key::Escape 至少需要是 256");

        if (glfwKey < GLFW_KEY_ESCAPE) {
            // 与定义相同，直接返回
            return (Key)glfwKey;
        }

        switch (glfwKey) {
        case GLFW_KEY_ESCAPE        : return Key::Escape;
        case GLFW_KEY_ENTER         : return Key::Enter;
        case GLFW_KEY_TAB           : return Key::Tab;
        case GLFW_KEY_BACKSPACE     : return Key::Backspace;
        case GLFW_KEY_INSERT        : return Key::Insert;
        case GLFW_KEY_DELETE        : return Key::Del;
        case GLFW_KEY_RIGHT         : return Key::Right;
        case GLFW_KEY_LEFT          : return Key::Left;
        case GLFW_KEY_DOWN          : return Key::Down;
        case GLFW_KEY_UP            : return Key::Up;
        case GLFW_KEY_PAGE_UP       : return Key::PageUp;
        case GLFW_KEY_PAGE_DOWN     : return Key::PageDown;
        case GLFW_KEY_HOME          : return Key::Home;
        case GLFW_KEY_END           : return Key::End;
        case GLFW_KEY_CAPS_LOCK     : return Key::CapsLock;
        case GLFW_KEY_SCROLL_LOCK   : return Key::ScrollLock;
        case GLFW_KEY_NUM_LOCK      : return Key::NumLock;
        case GLFW_KEY_PRINT_SCREEN  : return Key::PrintScreen;
        case GLFW_KEY_PAUSE         : return Key::Pause;
        case GLFW_KEY_F1            : return Key::F1;
        case GLFW_KEY_F2            : return Key::F2;
        case GLFW_KEY_F3            : return Key::F3;
        case GLFW_KEY_F4            : return Key::F4;
        case GLFW_KEY_F5            : return Key::F5;
        case GLFW_KEY_F6            : return Key::F6;
        case GLFW_KEY_F7            : return Key::F7;
        case GLFW_KEY_F8            : return Key::F8;
        case GLFW_KEY_F9            : return Key::F9;
        case GLFW_KEY_F10           : return Key::F10;
        case GLFW_KEY_F11           : return Key::F11;
        case GLFW_KEY_F12           : return Key::F12;
        case GLFW_KEY_KP_0          : return Key::Keypad0;
        case GLFW_KEY_KP_1          : return Key::Keypad1;
        case GLFW_KEY_KP_2          : return Key::Keypad2;
        case GLFW_KEY_KP_3          : return Key::Keypad3;
        case GLFW_KEY_KP_4          : return Key::Keypad4;
        case GLFW_KEY_KP_5          : return Key::Keypad5;
        case GLFW_KEY_KP_6          : return Key::Keypad6;
        case GLFW_KEY_KP_7          : return Key::Keypad7;
        case GLFW_KEY_KP_8          : return Key::Keypad8;
        case GLFW_KEY_KP_9          : return Key::Keypad9;
        case GLFW_KEY_KP_DECIMAL    : return Key::KeypadDel;
        case GLFW_KEY_KP_DIVIDE     : return Key::KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY   : return Key::KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT   : return Key::KeypadSubtract;
        case GLFW_KEY_KP_ADD        : return Key::KeypadAdd;
        case GLFW_KEY_KP_ENTER      : return Key::KeypadEnter;
        case GLFW_KEY_KP_EQUAL      : return Key::KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT    : return Key::LeftShift;
        case GLFW_KEY_LEFT_CONTROL  : return Key::LeftControl;
        case GLFW_KEY_LEFT_ALT      : return Key::LeftAlt;
        case GLFW_KEY_LEFT_SUPER    : return Key::LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT   : return Key::RightShift;
        case GLFW_KEY_RIGHT_CONTROL : return Key::RightControl;
        case GLFW_KEY_RIGHT_ALT     : return Key::RightAlt;
        case GLFW_KEY_RIGHT_SUPER   : return Key::RightSuper;
        case GLFW_KEY_MENU          : return Key::Menu;
        default                     : return Key::Unknown;
        }
    }

    inline static ModifierFlags GetModifierFlags(int modifiers)
    {
        ModifierFlags flags = ModifierFlags::None;
        if (modifiers & GLFW_MOD_ALT)
            flags |= ModifierFlags::Alt;
        if (modifiers & GLFW_MOD_CONTROL)
            flags |= ModifierFlags::Ctrl;
        if (modifiers & GLFW_MOD_SHIFT)
            flags |= ModifierFlags::Shift;
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

        modifiers = FixGLFWModifiers(modifiers, key, action);

        switch (action) {
        case GLFW_RELEASE : event.type = KeyboardEvent::Type::KeyReleased; break;
        case GLFW_PRESS   : event.type = KeyboardEvent::Type::KeyPressed; break;
        case GLFW_REPEAT  : event.type = KeyboardEvent::Type::KeyRepeated; break;
        default           : BEE_UNREACHABLE();
        }
        event.key  = GLFWToBeeKey(key);
        event.mods = GetModifierFlags(modifiers);

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

    glfwSetErrorCallback(ApiCallbacks::ErrorCallback);

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

void Window::setTitle(std::string_view title)
{
    glfwSetWindowTitle(_apiHandle, title.data());
}

void Window::setIcon(const std::filesystem::path& path)
{
    SetWindowIcon(path, _handle);
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
    BEE_ASSUME(width > 0 && height > 0);

    _desc.extent.x = width;
    _desc.extent.y = height;
}
