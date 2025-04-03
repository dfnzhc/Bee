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
#include <SDL3/SDL.h>

using namespace bee;

#define SDL_CHECK(expr)                                                                                                                              \
    do {                                                                                                                                             \
        if (!(expr)) {                                                                                                                               \
            BEE_THROW("SDL_Error: {}", SDL_GetError());                                                                                      \
        }                                                                                                                                            \
    } while (false)

#define SDL_UNEXPECTED(msg) Unexpected("{}\nSDL_Error: {}", msg, SDL_GetError())

namespace {
std::atomic<u16> sWindowCount = {0};
} // namespace

Window::Window(IWindowCallbacks* pCallbacks) : _pCallbacks(pCallbacks)
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
        switch (event.type) {
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_KEY_DOWN: break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_BUTTON_DOWN: break;
        case SDL_EVENT_MOUSE_MOTION: break;
        case SDL_EVENT_MOUSE_WHEEL: break;

        case SDL_EVENT_WINDOW_RESIZED: break;
        case SDL_EVENT_QUIT: _isRequestExit = true; break;
        default: break;
        }
    }
    // clang-format on

    SDL_SetRenderDrawColorFloat(_pRenderer, 0.2f, 0.3f, 0.7f, 1.0f);
    SDL_RenderClear(_pRenderer);
    SDL_RenderPresent(_pRenderer);
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
    SDL_CHECK(SDL_SetWindowSize(_pWindow, width, height));

    if (_pWindow)
        _pCallbacks->onWindowSizeChanged(width, height);
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
        return {{posX, posY}};

    return SDL_UNEXPECTED("Failed to get windows position.");
}

Result<f32> Window::dpiScale() const
{
    const auto dpi = SDL_GetWindowDisplayScale(_pWindow);
    if (dpi == 0.f) {
        return SDL_UNEXPECTED("Failed to get windows position.");
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
    handle = SDL_GetPointerProperty(SDL_GetWindowProperties(_pWindow),
                                    SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                    nullptr);
#endif

    if (handle)
        return handle;

    return SDL_UNEXPECTED("Failed to get native window handle.");
}