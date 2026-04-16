#include "games/finesweeper/Finesweeper.hpp"
#include "renderer/Renderer.hpp"

#include <imgui.h>

#include <algorithm>
#include <random>


static constexpr Color k_mine_count_colors[8] = {
    {0.0f, 0.0f, 1.0f, 1.0f}, // Blue
    {0.0f, 0.5f, 0.0f, 1.0f}, // Green
    {1.0f, 0.0f, 0.0f, 1.0f}, // Red
    {0.0f, 0.0f, 0.5f, 1.0f}, // Dark Blue
    {0.5f, 0.0f, 0.0f, 1.0f}, // Dark Red
    {0.0f, 0.5f, 0.5f, 1.0f}, // Cyan
    {0.0f, 0.0f, 0.0f, 1.0f}, // Black
    {0.5f, 0.5f, 0.5f, 1.0f}, // Gray
};


std::unique_ptr<Game>
Game::CreateFinesweeper()
{
    return std::make_unique<Finesweeper>();
}

Finesweeper::Finesweeper()
    : m_font{k_dejavu_sans_filepath, 22}
{
    float cell_size = 1.2f * std::min(m_world_height / k_max_grid_height, m_world_width / k_max_grid_width);
    float cell_size_without_border = 0.8f * cell_size;

    m_cell_outer_size = {cell_size, cell_size};
    m_cell_inner_size = {cell_size_without_border, cell_size_without_border};
}

void
Finesweeper::Start()
{
    m_cells_uncovered = 0;
    if (m_difficulty == beginner) {
        m_grid_width = 8;
        m_grid_height = 8;
        m_mine_count = 10;
    }
    else if (m_difficulty == intermediate) {
        m_grid_width = 16;
        m_grid_height = 16;
        m_mine_count = 40;
    }
    else {
        m_grid_width = 30;
        m_grid_height = 16;
        m_mine_count = 99;
    }
    assert(m_grid_width <= k_max_grid_width);
    assert(m_grid_height <= k_max_grid_height);


    float grid_draw_width = (float)m_grid_width * m_cell_outer_size.x;
    float grid_draw_height = (float)m_grid_height * m_cell_outer_size.y;
    m_grid_pos = {
        (m_world_width - grid_draw_width) / 2,
        (m_world_height - grid_draw_height) / 2,
    };


    memset(m_is_covered_bitmap, 0xff, sizeof(m_is_covered_bitmap));
    memset(m_is_flagged_bitmap, 0 , sizeof(m_is_flagged_bitmap));

    m_game_status = game_resume;
}

void
Finesweeper::InitIsMineBitmap(int32_t first_click_x, int32_t first_click_y)
{
    assert(m_mine_count < m_grid_width * m_grid_height - 2);
    memset(m_is_mine_bitmap, 0 , sizeof(m_is_mine_bitmap));

    std::mt19937 rng((std::random_device()()));
    std::uniform_int_distribution<int32_t> dist(0, m_grid_width * m_grid_height - 1);


    m_is_mine_bitmap[first_click_y] |= 1 << first_click_x;

    int32_t mine_count = m_mine_count;
    while (mine_count) {
        int32_t random_pos = dist(rng);
        int32_t y = random_pos / m_grid_width;
        int32_t x = random_pos % m_grid_width;
        if (!IsMine(x, y)) {
            m_is_mine_bitmap[y] |= 1 << x;
            mine_count--;
        }
    }

    m_is_mine_bitmap[first_click_y] &= ~(1u << first_click_x);
}

void
Finesweeper::InitAdjacentMineCounters()
{
    for (int32_t y = 0; y < m_grid_height; y++) {
        int32_t y0 = y > 0 ? y-1 : y;
        int32_t y1 = y < m_grid_height-1 ? y+1 : y;

        for (int32_t x = 0; x < m_grid_width; x++) {
            int32_t x0 = x > 0 ? x-1 : x;
            int32_t x1 = x < m_grid_width-1 ? x+1 : x;

            int32_t adjacent_mine_counter = 0;
            for (int32_t inner_y = y0; inner_y <= y1; inner_y++) {
                for (int32_t inner_x = x0; inner_x <= x1; inner_x++) {
                    if (IsMine(inner_x, inner_y)) {
                        adjacent_mine_counter++;
                    }
                }
            }
            if (IsMine(x, y)) {
                adjacent_mine_counter = -1;
            }

            m_adjacent_mine_counts[y * m_grid_width + x] = adjacent_mine_counter;
        }
    }
}

bool
Finesweeper::IsWon()
{
    bool is_won = m_grid_width*m_grid_height - m_cells_uncovered == m_mine_count;
    return is_won;
}

void
Finesweeper::ProcessEvent(SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        if (event.key.key == SDLK_ESCAPE) {
            m_game_status = game_pause;
        }
    } break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        V2F32 click_screen_pos = {event.button.x, (float)g_renderer.m_screen_h-1 - event.button.y};
        V2F32 click_view_pos = g_renderer.ScreenPosToViewPos(click_screen_pos);

        float x_adjusted = click_view_pos.x - m_grid_pos.x;
        float y_adjusted = click_view_pos.y - m_grid_pos.y;
        if (x_adjusted < 0.0f) {
            break;
        }
        if (y_adjusted < 0.0f) {
            break;
        }

        int32_t x = (int32_t)(x_adjusted / m_cell_outer_size.x);
        int32_t y = (int32_t)(y_adjusted / m_cell_outer_size.y);
        if (x >= m_grid_width) {
            break;
        }
        if (y >= m_grid_height) {
            break;
        }

        uint8_t left_click = 1;
        uint8_t right_click = 3;
        if (event.button.button == left_click) {
            if (m_cells_uncovered == 0) {
                InitIsMineBitmap(x, y);
                InitAdjacentMineCounters();
            }

            if (IsCovered(x, y)) {
                if (IsMine(x, y)) {
                    m_is_covered_bitmap[y] &= (uint32_t)~(1 << x);
                    UncoverMines();
                    m_game_status = game_over;
                }
                else {
                    Uncover(x, y);
                    if (IsWon()) {
                        m_game_status = game_over;
                    }
                }
            }
        }
        else if (event.button.button == right_click) {
            if (IsCovered(x, y)) {
                ToggleFlag(x ,y);
            }
        }

    } break;

    default:;
    }
}

void
Finesweeper::Update(float dt)
{
}

void
Finesweeper::Uncover(int32_t x, int32_t y)
{
    if (x < 0)              return;
    if (x >= m_grid_width)  return;
    if (y < 0)              return;
    if (y >= m_grid_height) return;
    if (!IsCovered(x, y))   return;

    m_is_covered_bitmap[y] &= (uint32_t)~(1 << x);
    m_cells_uncovered += 1;

    if (IsFlagged(x, y)) {
        ToggleFlag(x, y);
    }

    if (m_adjacent_mine_counts[y*m_grid_width + x] > 0) {
        return;
    }
    Uncover(x-1, y-1);
    Uncover(x  , y-1);
    Uncover(x+1, y-1);

    Uncover(x-1, y);
    Uncover(x+1, y);

    Uncover(x-1, y+1);
    Uncover(x  , y+1);
    Uncover(x+1, y+1);
}

void
Finesweeper::UncoverMines()
{
    for (int32_t y{0}; y < m_grid_height; ++y) {
        for (int32_t x{0}; x < m_grid_width; ++x) {
            if (IsMine(x, y) && IsCovered(x, y))  {
                m_is_covered_bitmap[y] &= (uint32_t)~(1 << x);
            }
        }
    }
}

void
Finesweeper::ToggleFlag(int32_t x, int32_t y)
{
    m_is_flagged_bitmap[y] ^= (1 << x);
}

bool
Finesweeper::IsCovered(int32_t x, int32_t y)
{
    bool is_covered = m_is_covered_bitmap[y] & 1 << x;
    return is_covered;
}

bool
Finesweeper::IsFlagged(int32_t x, int32_t y)
{
    bool is_flagged = m_is_flagged_bitmap[y] & 1 << x;
    return is_flagged;
}

bool
Finesweeper::IsMine(int32_t x, int32_t y)
{
    bool is_mine = m_is_mine_bitmap[y] & 1 << x;
    return is_mine;
}

void
Finesweeper::Draw()
{
    uint32_t z = 1;
    Color covered_cell_color {0.6f, 0.6f, 0.6f};
    Color uncovered_cell_color {0.4f, 0.4f, 0.4f};
    Color mine_color {0.8f, 0.2f, 0.2f};

    Color flag_color {0.6f, 0.3f, 03.f};
    V2F32 flag_size = {m_cell_inner_size.x * 0.5f, m_cell_inner_size.y * 0.5f};
    V2F32 flag_offset = {
        (m_cell_inner_size.x - flag_size.x) / 2,
        (m_cell_inner_size.y - flag_size.y) / 2
    };


    for (int32_t y = 0; y < m_grid_height; y++) {
        for (int32_t x = 0; x < m_grid_width; x++) {
            V2F32 cell_pos = {
                m_grid_pos.x + (float)x * m_cell_outer_size.x,
                m_grid_pos.y + (float)y * m_cell_outer_size.y,
            };
            AABB cell_aabb = {
                cell_pos.x, cell_pos.y,
                cell_pos.x + m_cell_inner_size.x, cell_pos.y + m_cell_inner_size.y
            };

            bool is_covered = IsCovered(x, y);
            bool is_flagged = IsFlagged(x, y);
            bool is_mine = IsMine(x, y);

            if (is_covered) {
                g_renderer.PushAABB(cell_aabb, covered_cell_color, 0.0f);


                if (is_flagged) {
                    V2F32 flag_pos = {
                        cell_pos.x + flag_offset.x,
                        cell_pos.y + flag_offset.y,
                    };
                    AABB flag_aabb = {
                        flag_pos.x,
                        flag_pos.y,
                        flag_pos.x + flag_size.x,
                        flag_pos.y + flag_size.y,
                    };
                    g_renderer.PushAABB(flag_aabb, flag_color, z);
                }
            }
            else {
                if (is_mine) {
                    V2F32 mine_pos = {
                        cell_pos.x,
                        cell_pos.y,
                    };
                    AABB mine_aabb = {
                        mine_pos.x,
                        mine_pos.y,
                        mine_pos.x + m_cell_inner_size.x,
                        mine_pos.y + m_cell_inner_size.y,
                    };
                    g_renderer.PushAABB(mine_aabb, mine_color, z);
                }
                else {
                    g_renderer.PushAABB(cell_aabb, uncovered_cell_color, 0.0f);

                    V2F32 mine_count_pos = {
                        cell_pos.x,
                        cell_pos.y,
                    };
                    int32_t mine_count = m_adjacent_mine_counts[y*m_grid_width + x];
                    if (mine_count > 0) {
                        Color color = k_mine_count_colors[mine_count-1];
                        Glyph& glyph = m_font.GetGlyph('0' + (char32_t)mine_count);
                        g_renderer.PushAlphaBitmap(glyph.bitmap, mine_count_pos, color, z);
                    }
                }
            }
        }
    }
}

void
Finesweeper::DrawGameStartMenu()
{
    ImGui::Begin("FinesweeperGameStartMenu");
    if (ImGui::RadioButton("beginner", m_difficulty == beginner ? true : false)) {
        m_difficulty = beginner;
    }
    if (ImGui::RadioButton("intermediate", m_difficulty == intermediate ? true : false)) {
        m_difficulty = intermediate;
    }
    if (ImGui::RadioButton("expert", m_difficulty == expert ? true : false)) {
        m_difficulty = expert;
    }
    if (ImGui::Button("Start")) {
        Start();
    }
    if (ImGui::Button("Exit")) {
        m_game_status = game_exit;
    }
    ImGui::End();
}

void
Finesweeper::DrawGameOverMenu()
{
    ImGui::Begin("FinesweeperGameOverMenu", nullptr, k_imgui_window_flags_menu);
    if (IsWon()) {
        ImGui::Text("You won!");
    }
    else {
        ImGui::Text("You Lost.");
    }
    if (ImGui::Button("Play Again")) {
        m_game_status = game_start;
    }
    if (ImGui::Button("Exit")) {
        m_game_status = game_exit;
    }
    ImGui::End();
}

