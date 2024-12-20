/**
 * @File Window.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#include "Base/Window.hpp"

#include <volk.h>
#include <Utility/Logger.hpp>
#include <Platform/Platform.hpp>
#include <SDL3/SDL_vulkan.h>

using namespace bee;

#define SDL_CHECK(expr)                                                                                                                              \
    do {                                                                                                                                             \
        if (!(expr)) {                                                                                                                               \
            BEE_ASSERT(false, "SDL_Error: {}", SDL_GetError());                                                                                      \
        }                                                                                                                                            \
    } while (false)

// std::vector<StringView> bee::SurfaceExtensions()
// {
//     SDL_GetExtension
//     u32 extCount = 0;
//     const char** glfwExtensions;
//     // glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);
//
//     return std::vector<StringView>{glfwExtensions, glfwExtensions + extCount};
// }

namespace bee {
namespace {

KeyboardEvent::Key SDLToBeeKey(SDL_Keycode sdlKey)
{
    switch (sdlKey) {
    case SDLK_SPACE        : return KeyboardEvent::Key::Space;
    case SDLK_APOSTROPHE   : return KeyboardEvent::Key::Apostrophe;
    case SDLK_COMMA        : return KeyboardEvent::Key::Comma;
    case SDLK_MINUS        : return KeyboardEvent::Key::Minus;
    case SDLK_PERIOD       : return KeyboardEvent::Key::Period;
    case SDLK_SLASH        : return KeyboardEvent::Key::Slash;
    case SDLK_0            : return KeyboardEvent::Key::Key0;
    case SDLK_1            : return KeyboardEvent::Key::Key1;
    case SDLK_2            : return KeyboardEvent::Key::Key2;
    case SDLK_3            : return KeyboardEvent::Key::Key3;
    case SDLK_4            : return KeyboardEvent::Key::Key4;
    case SDLK_5            : return KeyboardEvent::Key::Key5;
    case SDLK_6            : return KeyboardEvent::Key::Key6;
    case SDLK_7            : return KeyboardEvent::Key::Key7;
    case SDLK_8            : return KeyboardEvent::Key::Key8;
    case SDLK_9            : return KeyboardEvent::Key::Key9;
    case SDLK_SEMICOLON    : return KeyboardEvent::Key::Semicolon;
    case SDLK_EQUALS       : return KeyboardEvent::Key::Equal;
    case SDLK_A            : return KeyboardEvent::Key::A;
    case SDLK_B            : return KeyboardEvent::Key::B;
    case SDLK_C            : return KeyboardEvent::Key::C;
    case SDLK_D            : return KeyboardEvent::Key::D;
    case SDLK_E            : return KeyboardEvent::Key::E;
    case SDLK_F            : return KeyboardEvent::Key::F;
    case SDLK_G            : return KeyboardEvent::Key::G;
    case SDLK_H            : return KeyboardEvent::Key::H;
    case SDLK_I            : return KeyboardEvent::Key::I;
    case SDLK_J            : return KeyboardEvent::Key::J;
    case SDLK_K            : return KeyboardEvent::Key::K;
    case SDLK_L            : return KeyboardEvent::Key::L;
    case SDLK_M            : return KeyboardEvent::Key::M;
    case SDLK_N            : return KeyboardEvent::Key::N;
    case SDLK_O            : return KeyboardEvent::Key::O;
    case SDLK_P            : return KeyboardEvent::Key::P;
    case SDLK_Q            : return KeyboardEvent::Key::Q;
    case SDLK_R            : return KeyboardEvent::Key::R;
    case SDLK_S            : return KeyboardEvent::Key::S;
    case SDLK_T            : return KeyboardEvent::Key::T;
    case SDLK_U            : return KeyboardEvent::Key::U;
    case SDLK_V            : return KeyboardEvent::Key::V;
    case SDLK_W            : return KeyboardEvent::Key::W;
    case SDLK_X            : return KeyboardEvent::Key::X;
    case SDLK_Y            : return KeyboardEvent::Key::Y;
    case SDLK_Z            : return KeyboardEvent::Key::Z;
    case SDLK_LEFTBRACKET  : return KeyboardEvent::Key::LeftBracket;
    case SDLK_BACKSLASH    : return KeyboardEvent::Key::Backslash;
    case SDLK_RIGHTBRACKET : return KeyboardEvent::Key::RightBracket;
    case SDLK_GRAVE        : return KeyboardEvent::Key::GraveAccent;
    case SDLK_ESCAPE       : return KeyboardEvent::Key::Escape;
    case SDLK_TAB          : return KeyboardEvent::Key::Tab;
    case SDLK_RETURN       : return KeyboardEvent::Key::Enter;
    case SDLK_BACKSPACE    : return KeyboardEvent::Key::Backspace;
    case SDLK_INSERT       : return KeyboardEvent::Key::Insert;
    case SDLK_DELETE       : return KeyboardEvent::Key::Del;
    case SDLK_RIGHT        : return KeyboardEvent::Key::Right;
    case SDLK_LEFT         : return KeyboardEvent::Key::Left;
    case SDLK_DOWN         : return KeyboardEvent::Key::Down;
    case SDLK_UP           : return KeyboardEvent::Key::Up;
    case SDLK_PAGEUP       : return KeyboardEvent::Key::PageUp;
    case SDLK_PAGEDOWN     : return KeyboardEvent::Key::PageDown;
    case SDLK_HOME         : return KeyboardEvent::Key::Home;
    case SDLK_END          : return KeyboardEvent::Key::End;
    case SDLK_CAPSLOCK     : return KeyboardEvent::Key::CapsLock;
    case SDLK_SCROLLLOCK   : return KeyboardEvent::Key::ScrollLock;
    case SDLK_NUMLOCKCLEAR : return KeyboardEvent::Key::NumLock;
    case SDLK_PRINTSCREEN  : return KeyboardEvent::Key::PrintScreen;
    case SDLK_PAUSE        : return KeyboardEvent::Key::Pause;
    case SDLK_F1           : return KeyboardEvent::Key::F1;
    case SDLK_F2           : return KeyboardEvent::Key::F2;
    case SDLK_F3           : return KeyboardEvent::Key::F3;
    case SDLK_F4           : return KeyboardEvent::Key::F4;
    case SDLK_F5           : return KeyboardEvent::Key::F5;
    case SDLK_F6           : return KeyboardEvent::Key::F6;
    case SDLK_F7           : return KeyboardEvent::Key::F7;
    case SDLK_F8           : return KeyboardEvent::Key::F8;
    case SDLK_F9           : return KeyboardEvent::Key::F9;
    case SDLK_F10          : return KeyboardEvent::Key::F10;
    case SDLK_F11          : return KeyboardEvent::Key::F11;
    case SDLK_F12          : return KeyboardEvent::Key::F12;
    case SDLK_KP_0         : return KeyboardEvent::Key::Keypad0;
    case SDLK_KP_1         : return KeyboardEvent::Key::Keypad1;
    case SDLK_KP_2         : return KeyboardEvent::Key::Keypad2;
    case SDLK_KP_3         : return KeyboardEvent::Key::Keypad3;
    case SDLK_KP_4         : return KeyboardEvent::Key::Keypad4;
    case SDLK_KP_5         : return KeyboardEvent::Key::Keypad5;
    case SDLK_KP_6         : return KeyboardEvent::Key::Keypad6;
    case SDLK_KP_7         : return KeyboardEvent::Key::Keypad7;
    case SDLK_KP_8         : return KeyboardEvent::Key::Keypad8;
    case SDLK_KP_9         : return KeyboardEvent::Key::Keypad9;
    case SDLK_KP_DECIMAL   : return KeyboardEvent::Key::KeypadDel;
    case SDLK_KP_DIVIDE    : return KeyboardEvent::Key::KeypadDivide;
    case SDLK_KP_MULTIPLY  : return KeyboardEvent::Key::KeypadMultiply;
    case SDLK_KP_MINUS     : return KeyboardEvent::Key::KeypadSubtract;
    case SDLK_KP_PLUS      : return KeyboardEvent::Key::KeypadAdd;
    case SDLK_KP_ENTER     : return KeyboardEvent::Key::KeypadEnter;
    case SDLK_KP_EQUALS    : return KeyboardEvent::Key::KeypadEqual;
    case SDLK_LSHIFT       : return KeyboardEvent::Key::LeftShift;
    case SDLK_LCTRL        : return KeyboardEvent::Key::LeftControl;
    case SDLK_LALT         : return KeyboardEvent::Key::LeftAlt;
    case SDLK_LGUI         : return KeyboardEvent::Key::LeftSuper;
    case SDLK_RSHIFT       : return KeyboardEvent::Key::RightShift;
    case SDLK_RCTRL        : return KeyboardEvent::Key::RightControl;
    case SDLK_RALT         : return KeyboardEvent::Key::RightAlt;
    case SDLK_RGUI         : return KeyboardEvent::Key::RightSuper;
    case SDLK_MENU         : return KeyboardEvent::Key::Menu;
    default                : return KeyboardEvent::Key::Unknown;
    }
}

KeyboardEvent::ModifierFlags SDLToBeeModifier(SDL_Keymod sdlMod)
{
    BEE_USE_MAGIC_ENUM_BIT_OPERATOR;
    KeyboardEvent::ModifierFlags mod = {};

    if (sdlMod & SDL_KMOD_SHIFT)
        mod |= KeyboardEvent::ModifierFlags::Shift;

    if (sdlMod & SDL_KMOD_ALT)
        mod |= KeyboardEvent::ModifierFlags::Alt;

    if (sdlMod & SDL_KMOD_CTRL)
        mod |= KeyboardEvent::ModifierFlags::Ctrl;

    return mod;
}

} // namespace
} // namespace bee

class bee::ApiCallbacks
{
public:
    static void HandleWindowEvents(Window* pWindow, const SDL_WindowEvent& event)
    {
        if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            const auto width  = static_cast<u32>(event.data1);
            const auto height = static_cast<u32>(event.data2);

            pWindow->resize(width, height);
        }
    }

    static void HandleKeyboardEvent(Window* pWindow, const SDL_KeyboardEvent& event)
    {
        KeyboardEvent ke{};
        ke.key = SDLToBeeKey(event.key);

        if (event.type == SDL_EVENT_KEY_DOWN)
            ke.type = KeyboardEvent::Type::Pressed;
        else if (event.type == SDL_EVENT_KEY_UP)
            ke.type = KeyboardEvent::Type::Released;

        if (event.repeat)
            ke.type = KeyboardEvent::Type::Repeated;

        if (ke.key != KeyboardEvent::Key::Unknown)
            pWindow->_pCallbacks->handleKeyboardEvent(ke);
    }

    static void HandleMouseButtonEvent(Window* pWindow, const SDL_MouseButtonEvent& event)
    {
        MouseEvent me;
        switch (event.button) {
        case SDL_BUTTON_LEFT   : me.button = MouseEvent::Button::Left; break;
        case SDL_BUTTON_MIDDLE : me.button = MouseEvent::Button::Middle; break;
        case SDL_BUTTON_RIGHT  : me.button = MouseEvent::Button::Right; break;
        default                : return;
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            me.type = MouseEvent::Type::ButtonDown;
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            me.type = MouseEvent::Type::ButtonUp;

        me.screenPos  = {event.x, event.y};
        me.pos        = me.pos * pWindow->_getMouseScale();
        me.wheelDelta = {};
        pWindow->_pCallbacks->handleMouseEvent(me);
    }

    static void HandleMouseMotionEvent(Window* pWindow, const SDL_MouseMotionEvent& event)
    {
        MouseEvent me;
        me.type = MouseEvent::Type::Move;

        me.screenPos  = {event.x, event.y};
        me.pos        = me.pos * pWindow->_getMouseScale();
        me.wheelDelta = {};
        pWindow->_pCallbacks->handleMouseEvent(me);
    }

    static void HandleMouseWheelEvent(Window* pWindow, const SDL_MouseWheelEvent& event)
    {
        MouseEvent me;
        me.type = MouseEvent::Type::Wheel;

        me.screenPos  = {event.mouse_x, event.mouse_y};
        me.pos        = me.pos * pWindow->_getMouseScale();
        me.wheelDelta = {event.x, event.y};
        pWindow->_pCallbacks->handleMouseEvent(me);
    }
};

namespace {

std::atomic<u16> sWindowCount = {0};

} // namespace

Window::Window(const Window::Desc& desc, Window::ICallbacks* pCallbacks)
: _desc(desc), _pCallbacks(pCallbacks), _mouseScale(1.0f / (float)desc.extent.x, 1.0f / (float)desc.extent.y), _bIsRunning(true)
{
    LogInfo("Create window '{}'({}x{}).", desc.title, desc.extent.x, desc.extent.y);

    volkInitialize();
    if (sWindowCount.fetch_add(1) == 0) {
        SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));
    }

    // TODO: Handle errors
    vec2i extent = {static_cast<int>(desc.extent.x), static_cast<int>(desc.extent.y)};

    SDL_PropertiesID props{SDL_CreateProperties()};
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, extent.x);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, extent.y);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, desc.mode == Window::Mode::Fullscreen);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_MINIMIZED_BOOLEAN, desc.mode == Window::Mode::Minimized);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, desc.resizable);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, desc.mode == Window::Mode::Minimized);
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, desc.title.c_str());
    _apiHandle = SDL_CreateWindowWithProperties(props);

    BEE_ASSERT(_apiHandle, "Failed to create SDL window: {}", SDL_GetError());

    SDL_CHECK(SDL_SetWindowResizable(_apiHandle, desc.resizable));

    if (desc.mode == Window::Mode::Fullscreen) {
        SDL_CHECK(SDL_SetWindowFullscreen(_apiHandle, true));
        SDL_CHECK(SDL_GetWindowSize(_apiHandle, &extent.x, &extent.y));
    }
    else if (desc.mode == Window::Mode::Minimized) {
        SDL_CHECK(SDL_MinimizeWindow(_apiHandle));
    }

    SDL_CHECK(SDL_SetWindowFocusable(_apiHandle, desc.mode != Window::Mode::Minimized));

#ifdef BEE_IN_WINDOWS
    _handle = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(_apiHandle), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    BEE_ASSERT(_handle, "Failed to get Win32 window handle");
#endif

    setPos(_desc.pos);
    _updateWindowSize();

    SDL_CHECK(SDL_ShowWindow(_apiHandle));
}

Window::~Window()
{
    SDL_DestroyWindow(_apiHandle);

    if (sWindowCount.fetch_sub(1) == 1)
        SDL_Quit();
}

void Window::shutdown()
{
    _bIsRunning = false;
    SDL_CHECK(SDL_HideWindow(_apiHandle));
}

bool Window::shouldClose() const
{
    return !_bIsRunning;
}

void Window::resize(u32 width, u32 height)
{
    SDL_CHECK(SDL_SetWindowSize(_apiHandle, static_cast<int>(width), static_cast<int>(height)));

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
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_KEY_UP :
        case SDL_EVENT_KEY_DOWN          : ApiCallbacks::HandleKeyboardEvent(this, event.key); break;

        case SDL_EVENT_MOUSE_BUTTON_UP   :
        case SDL_EVENT_MOUSE_BUTTON_DOWN : ApiCallbacks::HandleMouseButtonEvent(this, event.button); break;
        case SDL_EVENT_MOUSE_MOTION      : ApiCallbacks::HandleMouseMotionEvent(this, event.motion); break;
        case SDL_EVENT_MOUSE_WHEEL       : ApiCallbacks::HandleMouseWheelEvent(this, event.wheel); break;

        case SDL_EVENT_WINDOW_RESIZED    : ApiCallbacks::HandleWindowEvents(this, event.window); break;
        default                          : break;
        }
    }
}

void Window::setPos(int x, int y)
{
    SDL_CHECK(SDL_SetWindowPosition(_apiHandle, x, y));
}

void Window::setTitle(std::string_view title)
{
    SDL_CHECK(SDL_SetWindowTitle(_apiHandle, title.data()));
}

void Window::setIcon(std::string_view path)
{
    SDL_Surface* icon = SDL_LoadBMP(path.data());
    if (icon == nullptr) {
        LogWarn("Failed to load icon from {}, SDL Error: {}", path, SDL_GetError());
        return;
    }

    SDL_CHECK(SDL_SetWindowIcon(_apiHandle, icon));
}

void Window::_updateWindowSize()
{
    int width, height;

    SDL_CHECK(SDL_GetWindowSize(_apiHandle, &width, &height));
    _setWindowSize(static_cast<u32>(width), static_cast<u32>(height));

    _mouseScale.x = 1.0f / static_cast<float>(width);
    _mouseScale.y = 1.0f / static_cast<float>(height);
}

void Window::_setWindowSize(u32 width, u32 height)
{
    BEE_CHECK(width > 0 && height > 0, "有效的窗口大小应该大于零");

    _desc.extent.x = width;
    _desc.extent.y = height;
}
