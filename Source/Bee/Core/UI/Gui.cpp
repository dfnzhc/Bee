/**
 * @File Gui.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/6
 * @Brief This file is part of Bee.
 */

#include "Core/UI/Gui.hpp"
#include "Base/Error.hpp"

#include <imgui.h>
#include <SDL3/SDL.h>

using namespace bee;

class bee::Gui::GuiImpl
{
public:
    friend class Gui;
    GuiImpl(SDL_Window* pWindow, SDL_Renderer* pRenderer, f32 dpiScale);

private:
    void init();

private:
    SDL_Window* _pWindow      = nullptr;
    SDL_Renderer* _pRenderer  = nullptr;
    Uint64 _pTime             = 0;
    char* _pClipboardTextData = nullptr;

    SDL_Cursor* _pMouseCursors[ImGuiMouseCursor_COUNT];
    SDL_Cursor* _pMouseLastCursor;
    ImGuiContext* _pContext = nullptr;

    SDL_WindowID _WindowID = 0;
    f32 _dpiScale          = 1.0f;

    bool _bMouseCanUseGlobalState        = false;
    bool _bMouseCanReportHoveredViewport = false;
};

Gui::GuiImpl::GuiImpl(SDL_Window* pWindow, SDL_Renderer* pRenderer, f32 dpiScale) : _pWindow(pWindow), _pRenderer(pRenderer), _dpiScale(dpiScale)
{
    BEE_ASSERT(_pWindow, "SDL_Window is invalid.");
    BEE_ASSERT(_pRenderer, "SDL_Renderer is invalid.");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    _pContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(_pContext);

    // IO
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    io.IniFilename  = nullptr;

    // styles
    ImGuiStyle& style                  = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg].w  = 0.9f;
    style.Colors[ImGuiCol_FrameBg].x  *= 0.1f;
    style.Colors[ImGuiCol_FrameBg].y  *= 0.1f;
    style.Colors[ImGuiCol_FrameBg].z  *= 0.1f;
    style.ScrollbarSize               *= 0.7f;

    style.Colors[ImGuiCol_MenuBarBg] = style.Colors[ImGuiCol_WindowBg];
    style.ScaleAllSizes(dpiScale);
}

void Gui::GuiImpl::init()
{
    // init sdl3 for imgui
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    BEE_ASSERT(io.BackendPlatformUserData == nullptr, "Already initialized a platform backend!");

    _bMouseCanUseGlobalState = false;
    const char* sdlBackend   = SDL_GetCurrentVideoDriver();
    for (auto& n : {"windows", "cocoa", "x11", "DIVE", "VMAN"})
        if (std::strncmp(sdlBackend, n, strlen(n)) == 0)
            _bMouseCanUseGlobalState = true;

    _bMouseCanReportHoveredViewport = _bMouseCanUseGlobalState;

    io.BackendPlatformUserData  = this;
    io.BackendPlatformName      = "Bee Imgui Impl";
    io.BackendFlags            |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags            |= ImGuiBackendFlags_HasSetMousePos;
    if (_bMouseCanUseGlobalState)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;

    _WindowID = SDL_GetWindowID(_pWindow);
}

Gui::Gui(void* pWindowSDL, void* pRendererSDL, f32 dpiScale)
{
    _impl = std::make_unique<GuiImpl>(static_cast<SDL_Window*>(pWindowSDL), static_cast<SDL_Renderer*>(pRendererSDL), dpiScale);
}