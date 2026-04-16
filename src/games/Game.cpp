#include "games/Game.hpp"
#include "common/defs.hpp"
#include "renderer/Renderer.hpp"

#include <assert.h>
#include <memory>


std::unique_ptr<Game>
Game::Select(GameType type)
{
    switch (type) {
    case no_game: {
        return nullptr;
    } break;

    case finesweeper: {
        return CreateFinesweeper();
    } break;

    case fnake: {
        return CreateFnake();
    } break;

    case fetris: {
        return CreateFetris();
    } break;

    case freakout: {
        return CreateFreakout();
    } break;

    case fasteroids: {
        return CreateFasteroids();
    } break;

    InvalidDefaultCase;
    }

    return nullptr;
}

float
Game::ProcessDt()
{
    uint64_t tnow_milliseconds = SDL_GetTicks();
    uint64_t tlast_milliseconds = m_tlast_milliseconds;
    uint64_t dt_milliseconds = tnow_milliseconds - tlast_milliseconds;

    float dt_seconds = float(dt_milliseconds) / 1000.0f;
    dt_seconds += m_dt_remaining_seconds;

    m_tlast_milliseconds = tnow_milliseconds;
    m_dt_remaining_seconds = 0.0f;

    return dt_seconds;
}

bool
Game::Update(std::vector<SDL_Event>& events)
{
    g_renderer.SetCameraSize(4.0f, 3.0f);
    g_renderer.SetClearColor(m_clear_color);

    float dt = ProcessDt();
    for (SDL_Event& event : events) {
        if (m_game_status == game_resume) {
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                m_game_status = game_pause;
            }
            else {
                ProcessEvent(event);
            }
        }
        else if (m_game_status == game_pause) {
            ProcessEventDuringPause(event);
        }
    }

    bool result = true;
    switch (m_game_status) {
    case game_start:  DrawGameStartMenu(); break;
    case game_resume: Update(dt); break;
    case game_over:   DrawGameOverMenu(); break;
    case game_pause:  DrawGamePauseMenu(); break;
    case game_exit:   result = false; break;
    InvalidDefaultCase;
    }

    if (m_game_status != game_start &&
        m_game_status != game_exit)
    {
        Draw();
    }

    return result;
}

void
Game::DrawGameStartMenu()
{
    Start(); // there is no menu, we just start the game.
}

void
Game::DrawGameOverMenu()
{
    ImGui::Begin("DefaultGameOverMenu", nullptr, k_imgui_window_flags_menu);
    ImGui::Text("Game Over.");
    if (ImGui::Button("Play Again")) {
        m_game_status = game_start;
    }
    if (ImGui::Button("Exit")) {
        m_game_status = game_exit;
    }
    ImGui::End();
}

void
Game::DrawGamePauseMenu()
{
    ImGui::Begin("DefaultGamePauseMenu", nullptr, k_imgui_window_flags_menu);
    if (ImGui::Button("Resume")) {
        m_game_status = game_resume;
    }
    if (ImGui::Button("Restart")) {
        m_game_status = game_start;
    }
    if (ImGui::Button("Exit")) {
        m_game_status = game_exit;
    }
    ImGui::End();
}

void
Game::ProcessEventDuringPause(SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        if (event.key.key == SDLK_ESCAPE) {
            m_game_status = game_resume;
        }
    }
    default:;
    }
}

