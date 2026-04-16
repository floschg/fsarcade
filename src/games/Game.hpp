#pragma once

#include "common/math.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <memory>
#include <vector>


class Game {
public:
    enum GameType {
        no_game,
        fetris,
        fnake,
        finesweeper,
        freakout,
        fasteroids
    };

    enum GameStatus {
        game_start,
        game_resume,
        game_over,
        game_pause,
        game_exit
    };


    using FrameString32 = uint32_t;


    static std::unique_ptr<Game> Select(GameType type);
    Game() = default;
    virtual ~Game() = default;

    bool Update(std::vector<SDL_Event>& events);


protected:
    GameStatus m_game_status {game_start};
    float m_dt_remaining_seconds {0.0f};
    uint64_t m_tlast_milliseconds {SDL_GetTicks()};
    Color m_clear_color {0.2f, 0.2f, 0.2f, 1.0f};


protected:
    virtual void Start() = 0;
    virtual void ProcessEvent(SDL_Event& event) = 0;
    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;

    virtual void DrawGameStartMenu();
    virtual void DrawGameOverMenu();


protected:
    static constexpr const char* k_dejavu_sans_filepath = "./fonts/dejavu_ttf/DejaVuSans.ttf";
    static constexpr const char* k_dejavu_sans_mono_filepath = "./fonts/dejavu_ttf/DejaVuSansMono.ttf";

    static constexpr ImGuiWindowFlags k_imgui_window_flags_menu = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize;
    static constexpr ImGuiWindowFlags k_imgui_window_flags_default = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar;


private:
    void DrawGamePauseMenu();
    void ProcessEventDuringPause(SDL_Event& event);
    float ProcessDt();

    static std::unique_ptr<Game> CreateFinesweeper();
    static std::unique_ptr<Game> CreateFnake();
    static std::unique_ptr<Game> CreateFetris();
    static std::unique_ptr<Game> CreateFreakout();
    static std::unique_ptr<Game> CreateFasteroids();
};

