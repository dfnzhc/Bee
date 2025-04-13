/**
 * @File Window.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/2
 * @Brief This file is part of Bee.
 */

#include "Core/Window.hpp"

#ifdef BEE_IN_WINDOWS
#  define SDL_MAIN_HANDLED
#endif

#include "Property.hpp"

#include <SDL3/SDL.h>

#include "Events/EventManager.hpp"
#include "Events/InputEvents.hpp"
#include "Events/FileEvents.hpp"
#include "Events/WindowEvents.hpp"
#include "Events/DispatchEvents.hpp"
#include "Math/Constant.hpp"

using namespace bee;

#define SDL_CHECK(expr)                                                                                                                                                                                \
    do {                                                                                                                                                                                               \
        if (!(expr)) {                                                                                                                                                                                 \
            BEE_THROW("SDL_Error: {}", SDL_GetError());                                                                                                                                                \
        }                                                                                                                                                                                              \
    } while (false)

#define SDL_UNEXPECTED(msg) Unexpected("{}\nSDL_Error: {}", msg, SDL_GetError())

namespace {
std::atomic<u16> sWindowCount = {0};

Keys SDLToBeeKey(SDL_Keycode sdlKey)
{
    // clang-format off
    switch (sdlKey) {
    case SDLK_SPACE        : return Keys::Space;
    case SDLK_APOSTROPHE   : return Keys::Apostrophe;
    case SDLK_COMMA        : return Keys::Comma;
    case SDLK_MINUS        : return Keys::Minus;
    case SDLK_PERIOD       : return Keys::Period;
    case SDLK_SLASH        : return Keys::Slash;
    case SDLK_0            : return Keys::Key0;
    case SDLK_1            : return Keys::Key1;
    case SDLK_2            : return Keys::Key2;
    case SDLK_3            : return Keys::Key3;
    case SDLK_4            : return Keys::Key4;
    case SDLK_5            : return Keys::Key5;
    case SDLK_6            : return Keys::Key6;
    case SDLK_7            : return Keys::Key7;
    case SDLK_8            : return Keys::Key8;
    case SDLK_9            : return Keys::Key9;
    case SDLK_SEMICOLON    : return Keys::Semicolon;
    case SDLK_EQUALS       : return Keys::Equal;
    case SDLK_A            : return Keys::A;
    case SDLK_B            : return Keys::B;
    case SDLK_C            : return Keys::C;
    case SDLK_D            : return Keys::D;
    case SDLK_E            : return Keys::E;
    case SDLK_F            : return Keys::F;
    case SDLK_G            : return Keys::G;
    case SDLK_H            : return Keys::H;
    case SDLK_I            : return Keys::I;
    case SDLK_J            : return Keys::J;
    case SDLK_K            : return Keys::K;
    case SDLK_L            : return Keys::L;
    case SDLK_M            : return Keys::M;
    case SDLK_N            : return Keys::N;
    case SDLK_O            : return Keys::O;
    case SDLK_P            : return Keys::P;
    case SDLK_Q            : return Keys::Q;
    case SDLK_R            : return Keys::R;
    case SDLK_S            : return Keys::S;
    case SDLK_T            : return Keys::T;
    case SDLK_U            : return Keys::U;
    case SDLK_V            : return Keys::V;
    case SDLK_W            : return Keys::W;
    case SDLK_X            : return Keys::X;
    case SDLK_Y            : return Keys::Y;
    case SDLK_Z            : return Keys::Z;
    case SDLK_LEFTBRACKET  : return Keys::LeftBracket;
    case SDLK_BACKSLASH    : return Keys::Backslash;
    case SDLK_RIGHTBRACKET : return Keys::RightBracket;
    case SDLK_GRAVE        : return Keys::GraveAccent;
    case SDLK_ESCAPE       : return Keys::Escape;
    case SDLK_TAB          : return Keys::Tab;
    case SDLK_RETURN       : return Keys::Enter;
    case SDLK_BACKSPACE    : return Keys::Backspace;
    case SDLK_INSERT       : return Keys::Insert;
    case SDLK_DELETE       : return Keys::Del;
    case SDLK_RIGHT        : return Keys::Right;
    case SDLK_LEFT         : return Keys::Left;
    case SDLK_DOWN         : return Keys::Down;
    case SDLK_UP           : return Keys::Up;
    case SDLK_PAGEUP       : return Keys::PageUp;
    case SDLK_PAGEDOWN     : return Keys::PageDown;
    case SDLK_HOME         : return Keys::Home;
    case SDLK_END          : return Keys::End;
    case SDLK_CAPSLOCK     : return Keys::CapsLock;
    case SDLK_SCROLLLOCK   : return Keys::ScrollLock;
    case SDLK_NUMLOCKCLEAR : return Keys::NumLock;
    case SDLK_PRINTSCREEN  : return Keys::PrintScreen;
    case SDLK_PAUSE        : return Keys::Pause;
    case SDLK_F1           : return Keys::F1;
    case SDLK_F2           : return Keys::F2;
    case SDLK_F3           : return Keys::F3;
    case SDLK_F4           : return Keys::F4;
    case SDLK_F5           : return Keys::F5;
    case SDLK_F6           : return Keys::F6;
    case SDLK_F7           : return Keys::F7;
    case SDLK_F8           : return Keys::F8;
    case SDLK_F9           : return Keys::F9;
    case SDLK_F10          : return Keys::F10;
    case SDLK_F11          : return Keys::F11;
    case SDLK_F12          : return Keys::F12;
    case SDLK_KP_0         : return Keys::Keypad0;
    case SDLK_KP_1         : return Keys::Keypad1;
    case SDLK_KP_2         : return Keys::Keypad2;
    case SDLK_KP_3         : return Keys::Keypad3;
    case SDLK_KP_4         : return Keys::Keypad4;
    case SDLK_KP_5         : return Keys::Keypad5;
    case SDLK_KP_6         : return Keys::Keypad6;
    case SDLK_KP_7         : return Keys::Keypad7;
    case SDLK_KP_8         : return Keys::Keypad8;
    case SDLK_KP_9         : return Keys::Keypad9;
    case SDLK_KP_DECIMAL   : return Keys::KeypadDel;
    case SDLK_KP_DIVIDE    : return Keys::KeypadDivide;
    case SDLK_KP_MULTIPLY  : return Keys::KeypadMultiply;
    case SDLK_KP_MINUS     : return Keys::KeypadSubtract;
    case SDLK_KP_PLUS      : return Keys::KeypadAdd;
    case SDLK_KP_ENTER     : return Keys::KeypadEnter;
    case SDLK_KP_EQUALS    : return Keys::KeypadEqual;
    case SDLK_LSHIFT       : return Keys::LeftShift;
    case SDLK_LCTRL        : return Keys::LeftControl;
    case SDLK_LALT         : return Keys::LeftAlt;
    case SDLK_LGUI         : return Keys::LeftSuper;
    case SDLK_RSHIFT       : return Keys::RightShift;
    case SDLK_RCTRL        : return Keys::RightControl;
    case SDLK_RALT         : return Keys::RightAlt;
    case SDLK_RGUI         : return Keys::RightSuper;
    case SDLK_MENU         : return Keys::Menu;
    default                : return Keys::Unknown;
    }
    // clang-format on
}

MouseButton SDLToBeeMouseButton(Uint8 sdlButton)
{
    // clang-format off
    switch (sdlButton) {
    case SDL_BUTTON_LEFT    : return MouseButton::Left;
    case SDL_BUTTON_MIDDLE  : return MouseButton::Middle;
    case SDL_BUTTON_RIGHT   : return MouseButton::Right;
    case SDL_BUTTON_X1      : return MouseButton::X1;
    case SDL_BUTTON_X2      : return MouseButton::X2;
    default                 : return MouseButton::Unknown;
    }
    // clang-format on
}

ModifierKeysState SDLCurrentModifier()
{
    const auto* keyStates = SDL_GetKeyboardState(nullptr);

    return {keyStates[SDL_SCANCODE_LSHIFT], keyStates[SDL_SCANCODE_RSHIFT], keyStates[SDL_SCANCODE_LCTRL], keyStates[SDL_SCANCODE_RCTRL], keyStates[SDL_SCANCODE_LALT], keyStates[SDL_SCANCODE_RALT]};
}

void HandleWindowEvents(const SDL_WindowEvent& event)
{
    const auto& em = EventManager::Instance();

    if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        em.Push(MakePtr<WindowResizeEvent>(event.data1, event.data2));
    }
}

void HandleKeyboardEvent(const SDL_KeyboardEvent& event)
{
    const auto& em = EventManager::Instance();

    const auto key = SDLToBeeKey(event.key);
    auto type      = InputType::Unknown;
    if (event.type == SDL_EVENT_KEY_DOWN)
        type = InputType::KeyDown;
    else if (event.type == SDL_EVENT_KEY_UP)
        type = InputType::KeyUp;

    const auto bIsRepeat = event.repeat;
    const auto modifiers = SDLCurrentModifier();

    em.Push(MakePtr<KeyboardEvent>(key, type, modifiers, bIsRepeat));
}

void HandleMouseButtonEvent(const SDL_MouseButtonEvent& event)
{
    auto& em = EventManager::Instance();

    const auto btn = SDLToBeeMouseButton(event.button);

    auto type = InputType::Unknown;
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        type = InputType::MouseButtonDown;
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        type = InputType::MouseButtonUp;

    const auto x         = event.x;
    const auto y         = event.y;
    const auto clicks    = event.clicks;
    const auto modifiers = SDLCurrentModifier();

    em.Push(MakePtr<MouseEvent>(x, y, btn, type, clicks, modifiers));
}

void HandleMouseMotionEvent(const SDL_MouseMotionEvent& event)
{
    auto& em = EventManager::Instance();

    const auto x         = event.x;
    const auto y         = event.y;
    const auto relX      = event.xrel;
    const auto relY      = event.yrel;
    const auto modifiers = SDLCurrentModifier();

    em.Push(MakePtr<MouseEvent>(x, y, relX, relY, modifiers));
}

void HandleMouseWheelEvent(const SDL_MouseWheelEvent& event)
{
    auto& em = EventManager::Instance();

    const auto h         = event.x;
    const auto v         = event.y;
    const auto modifiers = SDLCurrentModifier();

    em.Push(MakePtr<MouseEvent>(h, v, modifiers));
}

void HandleDropEvent(const SDL_DropEvent& event)
{
    auto& em = EventManager::Instance();

    if (event.type == SDL_EVENT_DROP_FILE) {
        em.Push(MakePtr<DropFileEvent>(event.data));
    }
}
} // namespace

Window::Window()
{
}

Window::~Window()
{
    SDL_DestroyRenderer(_pRenderer);
    SDL_DestroyWindow(_pWindow);

    if (sWindowCount.fetch_sub(1) == 1)
        SDL_Quit();
}

void Window::initialize()
{
    if (sWindowCount.fetch_add(1) == 0) {
        SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));
    }

    _mouseScale.x = 1.0f / cast_to<f32>(_width);
    _mouseScale.y = 1.0f / cast_to<f32>(_height);

    SDL_PropertiesID props{SDL_CreateProperties()};
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, _width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, _height);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Bee Engine");

    _pWindow = SDL_CreateWindowWithProperties(props);
    BEE_ASSERT(_pWindow, "Failed to create SDL window: {}", SDL_GetError());

    _pRenderer = SDL_CreateRenderer(_pWindow, nullptr);
    BEE_ASSERT(_pRenderer, "Failed to create SDL renderer: {}", SDL_GetError());

    setPos(50, 100); ///< Left-Top of the screen.
    show();
}

void Window::shutdown()
{
    _isRequestExit = true;
    SDL_CHECK(SDL_HideWindow(_pWindow));
}

void Window::pollForEvents()
{
    SDL_Event event;
    // clang-format off
    while (SDL_PollEvent(&event)) {
        EventManager::Instance().Dispatch(MakePtr<DispatchEvent>(DispatchEventType::Gui, static_cast<VoidPtr>(&event)));
        
        switch (event.type) {
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_KEY_DOWN          : HandleKeyboardEvent(event.key); break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_BUTTON_DOWN : HandleMouseButtonEvent(event.button); break;
        case SDL_EVENT_MOUSE_MOTION      : HandleMouseMotionEvent(event.motion); break;
        case SDL_EVENT_MOUSE_WHEEL       : HandleMouseWheelEvent(event.wheel); break;
            
        case SDL_EVENT_DROP_FILE         : HandleDropEvent(event.drop); break;

        case SDL_EVENT_WINDOW_RESIZED    : HandleWindowEvents(event.window); break;

        case SDL_EVENT_QUIT              : Property::RequestEngineExit(); break;
        default: break;
        }
    }
    // clang-format on
}

void Window::show() const
{
    SDL_CHECK(SDL_ShowWindow(_pWindow));
}

void Window::hide() const
{
    SDL_CHECK(SDL_HideWindow(_pWindow));
}

void Window::resize(int width, int height)
{
    _width  = width;
    _height = height;

    _mouseScale.x = 1.0f / cast_to<f32>(_width);
    _mouseScale.y = 1.0f / cast_to<f32>(_height);

    SDL_CHECK(SDL_SetWindowSize(_pWindow, width, height));
}

void Window::setPos(int posX, int posY) const
{
    SDL_CHECK(SDL_SetWindowPosition(_pWindow, posX, posY));
}

void Window::setTitle(StringView title) const
{
    SDL_CHECK(SDL_SetWindowTitle(_pWindow, title.data()));
}

void Window::setIcon(StringView path) const
{
    SDL_Surface* icon = SDL_LoadBMP(path.data());
    if (icon == nullptr) {
        LogWarn("Failed to load icon from {}, SDL Error: {}", path.data(), SDL_GetError());
        return;
    }

    SDL_CHECK(SDL_SetWindowIcon(_pWindow, icon));
}

void Window::setVSync(bool enabled) const
{
    int vsync = SDL_RENDERER_VSYNC_DISABLED;
    if (enabled)
        vsync = 1;

    SDL_CHECK(SDL_SetRenderVSync(_pRenderer, vsync));
}

vec2u Window::extent() const
{
    return {_width, _height};
}

u32 Window::width() const
{
    return _width;
}

u32 Window::height() const
{
    return _height;
}

Result<vec2i> Window::pos() const
{
    int posX, posY;
    if (SDL_GetWindowPosition(_pWindow, &posX, &posY))
        return {
          {posX, posY}
        };

    return SDL_UNEXPECTED("Failed to get windows position.");
}

f32 Window::dpiScale() const
{
    auto dpi = SDL_GetWindowDisplayScale(_pWindow);
    if (dpi == 0.f) {
        dpi = 1.f;
    }

    return dpi;
}

bool Window::isRequestExit() const
{
    return _isRequestExit;
}

VoidPtr Window::handleSDL() const
{
    return _pWindow;
}

Result<VoidPtr> Window::handleRaw() const
{
    VoidPtr handle = nullptr;

#ifdef BEE_IN_WINDOWS
    handle = SDL_GetPointerProperty(SDL_GetWindowProperties(_pWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif

    if (handle)
        return handle;

    return SDL_UNEXPECTED("Failed to get native window handle.");
}

VoidPtr Window::rendererSDL() const
{
    return _pRenderer;
}

vec2i Window::clampPos(int xPos, int yPos) const
{
    // TODO: clamp
    return {};
}

vec2f Window::clampPosNormalized(int xPos, int yPos) const
{
    // TODO: clamp
    return {};
}