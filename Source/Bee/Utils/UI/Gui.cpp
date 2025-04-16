/**
 * @File Gui.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/6
 * @Brief This file is part of Bee.
 */

#include "Utils/UI/Gui.hpp"
#include "Base/Error.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

using namespace bee;

class bee::Gui::Impl
{
public:
    friend class Gui;
    Impl(SDL_Window* pWindow, SDL_Renderer* pRenderer, f32 dpiScale);
    ~Impl();

    void beginFrame();
    void present();

    bool handleEvents(SDL_Event* pEventSDL) const;

private:
    void init();
    void shutdown();

private:
    SDL_Window* _pWindow     = nullptr;
    SDL_Renderer* _pRenderer = nullptr;
    ImGuiContext* _pContext  = nullptr;
    f32 _dpiScale            = 1.0f;
};

void Gui::Impl::beginFrame()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void Gui::Impl::present()
{
    constexpr auto clearColor = ImVec4(0.2f, 0.3f, 0.7f, 1.00f);

    // Rendering
    ImGui::Render();
    // SDL_RenderSetScale(_pRenderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(_pRenderer, clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    SDL_RenderClear(_pRenderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), _pRenderer);
    SDL_RenderPresent(_pRenderer);
}

bool Gui::Impl::handleEvents(SDL_Event* pEventSDL) const
{
    return ImGui_ImplSDL3_ProcessEvent(pEventSDL);
}

Gui::Impl::Impl(SDL_Window* pWindow, SDL_Renderer* pRenderer, f32 dpiScale) : _pWindow(pWindow), _pRenderer(pRenderer), _dpiScale(dpiScale)
{
    BEE_ASSERT(_pWindow, "SDL_Window is invalid.");
    BEE_ASSERT(_pRenderer, "SDL_Renderer is invalid.");

    init();
}

Gui::Impl::~Impl()
{
    shutdown();
}

void Gui::Impl::init()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    _pContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(_pContext);

    // IO
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.IniFilename  = nullptr;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(_pWindow, _pRenderer);
    ImGui_ImplSDLRenderer3_Init(_pRenderer);

    // Load font
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);
}

void Gui::Impl::shutdown()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

Gui::Gui(void* pWindowSDL, void* pRendererSDL, f32 dpiScale)
{
    _impl = std::make_unique<Impl>(static_cast<SDL_Window*>(pWindowSDL), static_cast<SDL_Renderer*>(pRendererSDL), dpiScale);
}

Gui::~Gui()
{
    if (_impl) {
        _impl.reset();
        _impl = nullptr;
    }
}

void Gui::beginFrame() const
{
    _impl->beginFrame();
}

void Gui::present() const
{
    bool show_demo_window    = true;
    bool show_another_window = false;
    ImVec4 clear_color       = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& io              = ImGui::GetIO();
    (void)io;

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f     = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window) {
        ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    _impl->present();
}

bool Gui::handleEvents(void* pEventSDL) const
{
    return _impl->handleEvents(static_cast<SDL_Event*>(pEventSDL));
    
}