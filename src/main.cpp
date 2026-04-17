#include "renderer/Renderer.hpp"
#include "games/Game.hpp"
#include "common/MemoryManager.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cstdlib>
#include <assert.h>
#include <chrono>


Game::GameType
DrawGameMenu()
{
    Game::GameType type = Game::no_game;
    ImGuiWindowFlags flags = 0;

    ImVec2 pos = {100, 200};
    ImGui::SetNextWindowPos(pos);


    ImGui::Begin("Game Selection", nullptr, flags);
    if (ImGui::Button("Fetris")) {
        type = Game::fetris;
    }
    if (ImGui::Button("Finesweeper")) {
        type = Game::finesweeper;
    }
    if (ImGui::Button("Fnake")) {
        type = Game::fnake;
    }
    if (ImGui::Button("Freakout")) {
        type = Game::freakout;
    }
    if (ImGui::Button("Fasteroids")) {
        type = Game::fasteroids;
    }
    ImGui::End();

    return type;
}


void
DrawPerformanceInfo(float ms_used)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

    float ms_per_frame = 1000.0f / io.Framerate;
    float load = (ms_used / ms_per_frame) * 100;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 350, 0));
    ImGui::Begin("Performance", nullptr, flags);
    ImGui::Text("%.2f ms/frame (%.2f %%) | %.2f ms/frame (%.f FPS)", ms_used, load, ms_per_frame, io.Framerate);
    ImGui::End();
}

int
main(int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 0;
    }

    SDL_Window* window;
    SDL_Renderer* sdl_renderer;
    SDL_WindowFlags window_flags = SDL_WINDOW_HIDDEN;
    if (!SDL_CreateWindowAndRenderer("fsarcade", 1280, 960, window_flags, &window, &sdl_renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 0;
    }
    SDL_SetRenderVSync(sdl_renderer, 1);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);



    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, sdl_renderer);
    ImGui_ImplSDLRenderer3_Init(sdl_renderer);



    g_renderer.Init(window, sdl_renderer);
    std::unique_ptr<Game> game = nullptr;

    std::vector<SDL_Event> game_events;
    game_events.reserve(32);

    for (;;) {
        auto start_time = std::chrono::high_resolution_clock::now();

        size_t cur_game_events = 0;
        size_t max_game_events = game_events.max_size();

        SDL_Event event;
        while (cur_game_events < max_game_events && SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_KEY_DOWN ||
                event.type == SDL_EVENT_KEY_UP ||
                event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                game_events.emplace_back(event);
                cur_game_events++;
            }
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
                return 0;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                return 0;
            if (event.type == SDL_EVENT_WINDOW_DESTROYED && event.window.windowID == SDL_GetWindowID(window)) {
                return 0;
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();



        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        g_renderer.SetScreenSize(w, h);
        g_renderer.SetClearColor({0.3f, 0.3f, 0.3f});


        if (game) {
            bool keep_game_running = game->Update(game_events);
            if (!keep_game_running) {
                game.reset();
            }
        }
        else {
            Game::GameType type = DrawGameMenu();
            if (type != Game::no_game) {
                game = Game::Select(type);
            }
        }


        game_events.clear();
        g_renderer.Draw();


        auto end_time = std::chrono::high_resolution_clock::now();
        float ms_used = float((end_time - start_time) / std::chrono::microseconds(1)) / 1000;
        DrawPerformanceInfo(ms_used);


        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), sdl_renderer);
        SDL_RenderPresent(sdl_renderer);


        g_renderer.Reset();
        MemoryManager::Clear_Frame();
    }

    return 0;
}

