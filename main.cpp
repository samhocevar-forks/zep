// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
#include <imgui.h>
#include "imgui/examples/sdl_opengl3_example/imgui_impl_sdl_gl3.h"
#include <stdio.h>
#include <imgui/examples/libs/gl3w/GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <SDL.h>

#undef max

#include "src/editor.h"
#include "src/display_imgui.h"
#include "src/syntax_glsl.h"
#include "src/mode_vim.h"

using namespace PicoVim;

const std::string shader = R"R(
#version 330 core

uniform mat4 Projection;

// Coordinates  of the geometry
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec4 in_color;

// Outputs to the pixel shader
out vec2 frag_tex_coord;
out vec4 frag_color;

void main()
{
    gl_Position = Projection * vec4(in_position.xyz, 1.0);
    frag_tex_coord = in_tex_coord;
    frag_color = in_color;
}
)R";

int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window *window = SDL_CreateWindow("PicoVim Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    gl3wInit();

    SDL_GL_SetSwapInterval(1);

    // Setup ImGui binding
    ImGui_ImplSdlGL3_Init(window);

    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };

    // Less packed font in X
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.DstFont = ImGui::GetFont();
    config.GlyphExtraSpacing.x = 1.0f;
    io.Fonts->AddFontFromFileTTF((std::string(SDL_GetBasePath()) + "/ProggyClean.ttf").c_str(), 13, &config, ranges);

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    /*
    class Client : public IPicoVimClient
    {
        virtual void Notify(std::shared_ptr<PicoVimMessage> message) {};
        virtual PicoVimEditor& GetEditor() const { return nullptr; }
        /*
        {
            return *spEditor;
        }
    };
*/
    auto spEditor = std::make_unique<PicoVimEditor>();
    auto spVimMode = std::make_unique<PicoVimMode_Vim>(*spEditor);
    spEditor->SetMode((PicoVimMode*)spVimMode.get());

    auto spDisplay = std::make_unique<PicoVimDisplayImGui>(*spEditor);

    PicoVimBuffer* pBuffer = spEditor->AddBuffer("shader.vert");

    // Syntax
    auto spSyntax = std::make_shared<PicoVimSyntaxGlsl>(*pBuffer);
    pBuffer->SetSyntax(std::static_pointer_cast<PicoVimSyntax>(spSyntax));

    // Set the text and store the buffer
    pBuffer->SetText(shader.c_str());

    // Temporary to ensure the correct window
    auto pWindow = spDisplay->GetCurrentWindow();
    if (pWindow)
    {
        if (pWindow->GetCurrentBuffer() == nullptr)
        {
            pWindow->SetCurrentBuffer(spEditor->GetBuffers()[0].get());
        }
    }

    // m_spEditor->RegisterCallback(this);

    // Main loop
    bool done = false;
    Timer lastChange;
    lastChange.Restart();
    while (!done)
    {
        SDL_Event event;
        if (SDL_WaitEventTimeout(&event, 50))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }
        else
        {
            if (!spDisplay->RefreshRequired())
            {
                continue;
            }
        }

        ImGui_ImplSdlGL3_NewFrame(window);

        bool open = true;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(.1f, .1f, .1f, 1.0f));

        auto size = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::Begin("PicoVim", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        spDisplay->SetDisplaySize(toNVec2f(ImGui::GetCursorScreenPos()),
           toNVec2f(size));

        spDisplay->Display();

        ImGui::End();
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(1);

        spEditor->GetMode()->SetCurrentWindow(spDisplay->GetCurrentWindow());
        auto& io = ImGui::GetIO();

        bool inputChanged = false;
        for (int n = 0; n < (sizeof(io.InputCharacters) / sizeof(*io.InputCharacters)) && io.InputCharacters[n]; n++)
        {
            spEditor->GetMode()->AddKeyPress(io.InputCharacters[n]);
        }

        if (io.KeyCtrl)
        {
            for (char ch = ' '; ch <= '~'; ch++)
            {
                if (ImGui::IsKeyPressed(int(ch)))
                {
                    spEditor->GetMode()->AddKeyPress(ch, ModifierKey::Ctrl);
                }
            }
        }

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
        {
            spEditor->GetMode()->AddKeyPress(ExtKeys::TAB);
        }
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        {
            spEditor->GetMode()->AddKeyPress(ExtKeys::ESCAPE);
        }
        else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
        {
            spEditor->GetMode()->AddKeyPress(ExtKeys::RETURN);
        }
        else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
        {
            spEditor->GetMode()->AddKeyPress(ExtKeys::DEL);
        }
        else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
        {
            spEditor->GetMode()->AddKeyPress(ExtKeys::BACKSPACE);
        }
        else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
        {
            spEditor->GetMode()->AddKeyPress(ExtKeys::RIGHT);
        }
        else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
        {
            spEditor->GetMode()->AddKeyPress(ExtKeys::LEFT);
        }
        else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
        {
            spEditor->GetMode()->AddKeyPress(ExtKeys::UP);
        }
        else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
        {
            spEditor->GetMode()->AddKeyPress(ExtKeys::DOWN);
        }

        // Rendering
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
