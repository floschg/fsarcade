#include "games/Game.hpp"
#include "games/fetris/Fetromino.hpp"
#include "games/fetris/Fetris.hpp"
#include "renderer/Renderer.hpp"
#include "common/MemoryManager.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>
#include <imgui.h>

#include <fstream>
#include <iostream>
#include <string>


std::unique_ptr<Game>
Game::CreateFetris()
{
    return std::make_unique<Fetris>();
}

Fetris::Fetris()
    : m_font{k_dejavu_sans_mono_filepath, 22}
    , m_active_fetromino{m_board.m_bitmap}
    , m_highscore {ReadHighscore()}
{
    Start();
}

void
Fetris::Start()
{
    m_tlast_milliseconds = SDL_GetTicks();
    m_dt_remaining_seconds = 0.0f;

    m_board.Reset();
    m_active_fetromino.Reset(Fetromino::GenerateRandomId());
    m_next_fetromino_id = Fetromino::GenerateRandomId();

    memset(m_fetromino_counters, 0, sizeof(m_fetromino_counters));
    m_fetromino_counters[m_active_fetromino.GetId()] += 1;
    m_score = 0;
    m_line_counter = 0;
    m_starting_level = 0;
    m_level = 0;
    m_softdrop_counter = 0;

    m_game_status = game_resume;
}

void
Fetris::Update(float dt)
{
    if (m_game_status == game_resume) {
        uint32_t softdrop_count = GetSoftdropCount(dt);
        for (uint32_t i{0}; i < softdrop_count; i++) {
            bool moved_down = m_active_fetromino.MaybeMoveDown();
            if (!moved_down) {
                HandleFetrominoPlacement();
            }
        }
    }
}

void
Fetris::ProcessEvent(SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        auto key = event.key.key;
        if (key == SDLK_RIGHT) {
            m_active_fetromino.MaybeMoveHorizontally(Fetromino::right);
        }
        else if (key == SDLK_LEFT) {
            m_active_fetromino.MaybeMoveHorizontally(Fetromino::left);
        }
        else if (key == SDLK_DOWN) {
            bool moved_down = m_active_fetromino.MaybeMoveDown();
            if (!moved_down) {
                HandleFetrominoPlacement();
            }
            else {
                m_softdrop_counter++;
            }
        }
        else if (key == SDLK_X) {
            m_active_fetromino.MaybeRotate(Fetromino::rotate_clockwise);
        }
        else if (key == SDLK_Z || key == SDLK_Y) {
            m_active_fetromino.MaybeRotate(Fetromino::rotate_counter_clockwise);
        }
    }
    default:;
    }
}

void
Fetris::HandleFetrominoPlacement()
{
    int32_t rows_cleared = m_board.PlaceFetromino(m_active_fetromino);


    m_active_fetromino.Reset(m_next_fetromino_id);
    m_next_fetromino_id = Fetromino::GenerateRandomId();


    if (m_active_fetromino.IsCollisionWithBoard()) {
        m_game_status = game_over;
        if (m_score > m_highscore) {
            m_highscore = m_score;
            WriteHighscore();
        }
    }


    m_line_counter += rows_cleared;
    m_fetromino_counters[m_active_fetromino.GetId()] += 1;

    if (rows_cleared == 1) {
        m_score += 40 * (m_level + 1);
    }
    else if (rows_cleared == 2) {
        m_score += 100 * (m_level + 1);
    }
    else if (rows_cleared == 3) {
        m_score += 300 * (m_level + 1);
    }
    else if (rows_cleared == 4) {
        m_score += 1200 * (m_level + 1);
    }

    m_score += m_softdrop_counter;
    m_softdrop_counter = 0;
    m_level = m_starting_level + m_line_counter / 10;
}

uint32_t
Fetris::GetSoftdropCount(float dt)
{
    float frame_time = 1.0f / 60;
    int32_t frames_per_cell;
    if      (m_level <= 8)  frames_per_cell = 48 - m_level * 5;
    else if (m_level == 9)  frames_per_cell = 6;
    else if (m_level <= 12) frames_per_cell = 5;
    else if (m_level <= 15) frames_per_cell = 4;
    else if (m_level <= 18) frames_per_cell = 3;
    else if (m_level <= 28) frames_per_cell = 2;
    else                    frames_per_cell = 1;


    float dt_level = static_cast<float>(frames_per_cell) * frame_time;

    uint32_t softdrop_count = 0;
    while (dt > dt_level) {
        softdrop_count += 1;
        dt -= dt_level;
    }


    m_dt_remaining_seconds = dt;
    return softdrop_count;
}

int32_t
Fetris::ReadHighscore()
{
    int32_t highscore = 0;
    std::ifstream highscore_file_in {k_highscore_filepath};
    if (highscore_file_in) {
        highscore_file_in >> highscore;
        highscore_file_in.close();
    }
    return highscore;
}

void
Fetris::WriteHighscore()
{
    std::ofstream highscore_file_out {k_highscore_filepath};
    if (highscore_file_out) {
        highscore_file_out << m_highscore << std::endl;
        highscore_file_out.close();
    }
    else {
        SDL_LogInfo(0, "Fetris: cannot open %s for writing", k_highscore_filepath);
    }
}

void
Fetris::Draw()
{
    m_board.Draw(m_level);
    m_active_fetromino.Draw();

    DrawNextFetromino();
    DrawStatistics();
    DrawLineCounter();
    DrawLevel();
    DrawScore();
}

void
Fetris::DrawGameOverMenu()
{
    ImGui::Begin("FetrisGameOver", nullptr, k_imgui_window_flags_menu);
    ImGui::Text("Score = %d", m_score);
    ImGui::Text("HighScore = %d", m_highscore);
    if (ImGui::Button("Restart")) {
        Start();
    }
    if (ImGui::Button("Exit")) {
        m_game_status = game_exit;
    }
    ImGui::End();
}

void
Fetris::DrawLineCounter()
{
    V2F32 pos = {0.5f, 2.6f};
    Color color = {0.9f, 0.9f, 0.9f, 1.0f};


    String32Id str_id = MemoryManager::EmplaceString32_Frame(U"Lines: xxx");
    std::u32string& str = MemoryManager::GetString32(str_id);
    int line_count = std::min(m_line_counter, 999);

    str[9] = U'0' + char32_t(line_count % 10);
    line_count /= 10;

    str[8] = U'0' + char32_t(line_count % 10);
    line_count /= 10;

    str[7] = U'0' + char32_t(line_count % 10);
    line_count /= 10;


    g_renderer.PushString32(str_id, m_font, pos, color, k_z_text);
    pos.x += 0.2f;
}

void
Fetris::DrawStatistics()
{
    V2F32 pos = {0.4f, 0.5f};


    String32Id title_text = MemoryManager::EmplaceString32_Frame(U"Statistics");
    V2F32 title_pos = {pos.x + 0.02f, pos.y + 1.64f};
    g_renderer.PushString32(title_text, m_font, title_pos, k_text_color, k_z_text);

    float yadvance = -0.2f;
    float fetrominoes_x0 = pos.x;
    float fetrominoes_y0 = pos.y - (float)Fetromino::id_count * yadvance;
    V2F32 fetromino_pos = {fetrominoes_x0, fetrominoes_y0};

    Fetromino::Draw(Fetromino::t_piece, 0, fetromino_pos, 0.5f);
    fetromino_pos.y += yadvance;

    Fetromino::Draw(Fetromino::j_piece, 0, fetromino_pos, 0.5f);
    fetromino_pos.y += yadvance;

    Fetromino::Draw(Fetromino::z_piece, 0, fetromino_pos, 0.5f);
    fetromino_pos.y += yadvance;

    Fetromino::Draw(Fetromino::o_piece, 0, fetromino_pos, 0.5f);
    fetromino_pos.y += yadvance;

    Fetromino::Draw(Fetromino::s_piece, 0, fetromino_pos, 0.5f);
    fetromino_pos.y += yadvance;

    Fetromino::Draw(Fetromino::l_piece, 0, fetromino_pos, 0.5f);
    fetromino_pos.y += yadvance;

    Fetromino::Draw(Fetromino::i_piece, 0, fetromino_pos, 0.5f);
    fetromino_pos.y += yadvance;


    // Todo: reorder fetrominoes' bitmaps and just for-loop this?

    float counters_x0 = pos.x + 0.4f;
    float counters_y0 = pos.y + 0.05f - yadvance * (float)Fetromino::id_count;
    V2F32 counters_pos = {counters_x0, counters_y0};


    String32Id t_count = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_fetromino_counters[Fetromino::t_piece]));
    String32Id j_count = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_fetromino_counters[Fetromino::j_piece]));
    String32Id z_count = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_fetromino_counters[Fetromino::z_piece]));
    String32Id o_count = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_fetromino_counters[Fetromino::o_piece]));
    String32Id s_count = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_fetromino_counters[Fetromino::s_piece]));
    String32Id l_count = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_fetromino_counters[Fetromino::l_piece]));
    String32Id i_count = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_fetromino_counters[Fetromino::i_piece]));

    g_renderer.PushString32(t_count, m_font, counters_pos, k_text_color, k_z_text);
    counters_pos.y += yadvance;
    g_renderer.PushString32(j_count, m_font, counters_pos, k_text_color, k_z_text);
    counters_pos.y += yadvance;
    g_renderer.PushString32(z_count, m_font, counters_pos, k_text_color, k_z_text);
    counters_pos.y += yadvance;
    g_renderer.PushString32(o_count, m_font, counters_pos, k_text_color, k_z_text);
    counters_pos.y += yadvance;
    g_renderer.PushString32(s_count, m_font, counters_pos, k_text_color, k_z_text);
    counters_pos.y += yadvance;
    g_renderer.PushString32(l_count, m_font, counters_pos, k_text_color, k_z_text);
    counters_pos.y += yadvance;
    g_renderer.PushString32(i_count, m_font, counters_pos, k_text_color, k_z_text);
}

void
Fetris::DrawScore()
{
    V2F32 pos = {3.0f, 2.6f};


    String32Id top_label = MemoryManager::EmplaceString32_Frame(U"Top");
    String32Id top_value = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_highscore));

    String32Id score_label = MemoryManager::EmplaceString32_Frame(U"Score");
    String32Id score_value = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_score));


    g_renderer.PushString32(top_label, m_font, pos, k_text_color, k_z_text);
    pos.y -= 0.1f;
    g_renderer.PushString32(top_value, m_font, pos, k_text_color, k_z_text);
    pos.y -= 0.2f;

    g_renderer.PushString32(score_label, m_font, pos, k_text_color, k_z_text);
    pos.y -= 0.1f;
    g_renderer.PushString32(score_value, m_font, pos, k_text_color, k_z_text);
}

void
Fetris::DrawNextFetromino()
{
    V2F32 pos = {3.0f, 1.4f};


    V2F32 label_pos = {pos.x, pos.y + 0.4f};
    String32Id label_text = MemoryManager::EmplaceString32_Frame(U"Next:");
    g_renderer.PushString32(label_text, m_font, label_pos, k_text_color, k_z_text);


    V2F32 fetromino_pos = {pos.x, pos.y};
    Fetromino::Draw(m_next_fetromino_id, 0, fetromino_pos, 0.5f);
}

void
Fetris::DrawLevel()
{
    V2F32 pos = {3.0f, 1.1f};

    String32Id label = MemoryManager::EmplaceString32_Frame(U"Level");
    g_renderer.PushString32(label, m_font, pos, k_text_color, k_z_text);
    pos.y -= 0.1f;

    String32Id level = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_level));
    g_renderer.PushString32(level, m_font, pos, k_text_color, k_z_text);
}

