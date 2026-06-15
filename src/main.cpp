#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "kernel/kernel.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include <emscripten.h>

using namespace std;

struct State {SDL_Window* window = nullptr; SDL_GLContext gl = nullptr;};
static State g_state;
static Kernel g_kernel;

extern "C" EMSCRIPTEN_KEEPALIVE void mysteryos_anomaly_response(const char* text) {
    g_kernel.receive_anomaly_response(text ? text : "");
}

static void loop(){
    SDL_Event e;
    while (SDL_PollEvent(&e)) ImGui_ImplSDL2_ProcessEvent(&e);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    g_kernel.render();
    ImGui::Render();
    int w, h;
    SDL_GetWindowSize(g_state.window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_state.window);
}

int main(){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    g_state.window = SDL_CreateWindow("MysteryOS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    g_state.gl = SDL_GL_CreateContext(g_state.window);
    SDL_GL_MakeCurrent(g_state.window, g_state.gl);
    SDL_GL_SetSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(g_state.window, g_state.gl);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    if (!g_kernel.init()) {
        emscripten_run_script("console.error('MysteryOS: kernel init failed — data files not found')");
    }
    emscripten_set_main_loop(loop, 0, true);
    return 0;
}
