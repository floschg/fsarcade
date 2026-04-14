#include "games/fnake/Fnake.hpp"
#include "common/shapes.hpp"
#include "games/Game.hpp"
#include "renderer/Renderer.hpp"
#include "common/MemoryManager.hpp"
#include "common/math.hpp"

#include <imgui.h>

#include <cstdint>
#include <fstream>
#include <iostream>


Fnake::Fnake()
    : m_font {k_dejavu_sans_mono_filepath, 22}
    , m_rng {std::random_device{}()}
{
    static_assert(k_tiles_x <= sizeof(m_body_bitmap[0])*8);
    static_assert(k_tiles_y <= sizeof(m_body_bitmap[0])*8);

    int32_t highscore = 0;
    std::ifstream highscore_file_in {"fnake_highscore.txt"};
    if (highscore_file_in) {
        highscore_file_in >> highscore;
        highscore_file_in.close();
    }
    m_highscore = highscore;
}

void
Fnake::Start()
{
    m_game_status = game_resume;
    m_tlast_milliseconds = SDL_GetTicks();

    m_curr_direction = right;
    m_next_direction = right;
    m_tile_progress = 0.0f;

    int32_t head_x = k_tiles_x / 2;
    int32_t head_y = k_tiles_y / 2;
    m_body_parts.clear();
    m_body_parts.emplace_front(V2I32{head_x-1, head_y}, right, none);
    m_body_parts.emplace_front(V2I32{head_x, head_y}, none, left);

    memset(m_body_bitmap, 0, sizeof(m_body_bitmap));
    m_body_bitmap[head_y] |= (1 << head_x);
    m_body_bitmap[head_y] |= (1 << (head_x-1));

    SpawnFood();

    m_score = 0;
}

void
Fnake::Update(float dt)
{
    MoveBody(dt);
}

void
Fnake::MoveBody(float dt)
{
    float tile_progress = m_tile_progress + k_tiles_per_second * dt;
    while (tile_progress >= 1.0f) {
        V2I32 head_pos = m_body_parts.front().tile_pos;
        V2I32 tail_pos = m_body_parts.back().tile_pos;


        // find next head_pos
        if (m_next_direction == up) {
            head_pos.y += 1;
        }
        else if (m_next_direction == down) {
            head_pos.y -= 1;
        }
        else if (m_next_direction == right) {
            head_pos.x += 1;
        }
        else if (m_next_direction == left) {
            head_pos.x -= 1;
        }
        if ((head_pos.x < 0 || head_pos.x >= k_tiles_x) ||
            (head_pos.y < 0 || head_pos.y >= k_tiles_y))
        {
            HandleGameOver();
            return;
        }


        // check collision
        uint64_t head_bit = 1 << head_pos.x;
        uint64_t body_bits = m_body_bitmap[head_pos.y];
        if (head_pos.y == tail_pos.y) {
            body_bits &= (uint32_t)~(1 << tail_pos.x);
        }
        if (head_bit & body_bits) {
            HandleGameOver();
            return;
        }


        // advance head
        m_body_bitmap[head_pos.y] |= (1 << head_pos.x);
        m_body_parts.front().next_direction_to_head = m_next_direction;
        m_body_parts.emplace_front(head_pos, none, (Direction)-m_next_direction);


        if (m_body_parts.front().tile_pos == m_food_tile_pos) {
            // eat food
            m_score += 1;
            SpawnFood();
            m_body_parts.back().next_direction_to_tail = none;
        }
        else {
            // advance tail
            auto vanishing_tail = m_body_parts.back();
            m_body_bitmap[tail_pos.y] &= (uint32_t)~(1 << tail_pos.x);
            m_body_parts.pop_back();
            m_body_parts.back().next_direction_to_tail = (Direction)-vanishing_tail.next_direction_to_head;
        }


        m_curr_direction = m_next_direction;
        tile_progress -= 1.0f;
    }

    m_tile_progress = tile_progress;
}

void
Fnake::HandleGameOver()
{
    m_game_status = game_over;
    if (m_score > m_highscore) {
        m_highscore = m_score;

        std::ofstream highscore_file_out {k_highscore_path};
        if (highscore_file_out) {
            highscore_file_out << m_highscore << std::endl;
            highscore_file_out.close();
        }
        else {
            SDL_LogInfo(0, "Fnake: cannot open %s for writing", k_highscore_path);
        }
    }
}

void
Fnake::ProcessEvent(SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        if (event.key.key == SDLK_UP) {
            if (m_curr_direction == right ||
                m_curr_direction == left)
            {
                m_next_direction = up;
            }
        }
        else if (event.key.key == SDLK_DOWN) {
            if (m_curr_direction == right ||
                m_curr_direction == left)
            {
                m_next_direction = down;
            }
        }
        else if (event.key.key == SDLK_RIGHT) {
            if (m_curr_direction == up ||
                m_curr_direction == down)
            {
                m_next_direction = right;
            }
        }
        else if (event.key.key == SDLK_LEFT) {
            if (m_curr_direction == up ||
                m_curr_direction == down)
            {
                m_next_direction = left;
            }
        }
    }

    default:;
    }
}

void
Fnake::SpawnFood()
{
    int32_t bit0_counts[k_tiles_y];
    int32_t bit0_count_total = 0;

    // count bits
    for (int32_t y = 0; y < k_tiles_y; y++) {
        int32_t bit1_count = 0;

        uint64_t bitmap_row = m_body_bitmap[y];
        while (bitmap_row != 0) {
            bitmap_row = bitmap_row & (bitmap_row - 1);
            bit1_count += 1;
        }

        int32_t bit0_count = k_tiles_x - bit1_count;
        bit0_counts[y] = bit0_count;
        bit0_count_total += bit0_count;
    }

    if (bit0_count_total == 0) {
        return;
    }

    m_dist.param(std::uniform_int_distribution<int32_t>::param_type(0, bit0_count_total - 1));
    int32_t bit0_index = m_dist(m_rng);
    int32_t bit0_x = 0;
    int32_t bit0_y = 0;

    // find y
    for (int32_t y = 0; y < k_tiles_y; y++) {
        if (bit0_index < bit0_counts[y]) {
            bit0_y = y;
            break;
        }
        bit0_index -= bit0_counts[y];
    }

    // find x
    uint64_t bitmap_row_not = ~m_body_bitmap[bit0_y];
    for (int32_t x = 0; x < k_tiles_x; x++) {
        if (bitmap_row_not & 1) {
            if (bit0_index == 0) {
                bit0_x = x;
                break;
            }
            bit0_index--;
        }
        bitmap_row_not >>= 1;
    }

    m_food_tile_pos = {bit0_x, bit0_y};
}

void
Fnake::Draw()
{
    Color tilemap_color = {0.0f, 0.0f, 0.0f, 1.0f};
    Rectangle tilemap_rect = {
        k_tilemap_x,
        k_tilemap_y,
        k_tilemap_x + k_tile_size * k_tiles_x,
        k_tilemap_y + k_tile_size * k_tiles_y
    };
    g_renderer.PushRectangle(tilemap_rect, tilemap_color, k_z_tilemap);


    DrawFood();
    DrawBody();
    DrawScores();
}

void
Fnake::DrawBody()
{
    Color body_color = {0.0f, 0.5f, 0.0f, 1.0f};
    float bodypart_size = 0.8f * k_tile_size;
    float bodypart_offset = (k_tile_size - bodypart_size) / 2;

    auto curr = m_body_parts.begin();
    auto next = curr + 1;

    Rectangle rect_curr;
    Rectangle rect_next;


    // draw head
    {
        float x0_next = TilemapXToWorldX(next->tile_pos.x) + bodypart_offset;
        float y0_next = TilemapYToWorldY(next->tile_pos.y) + bodypart_offset;
        rect_next = {
            x0_next,
            y0_next,
            x0_next + bodypart_size,
            y0_next + bodypart_size
        };
        rect_curr = rect_next;

        float progress_size = m_tile_progress * k_tile_size;
        if (curr->next_direction_to_tail == up) {
            rect_curr.y0 -= progress_size;
            rect_curr.y1 -= progress_size;
            if (rect_curr.y1 <= rect_next.y0) {
                DrawBodyConnectionUp(rect_curr, rect_next.y0);
            }
        }
        else if (curr->next_direction_to_tail == down) {
            rect_curr.y0 += progress_size;
            rect_curr.y1 += progress_size;
            if (rect_curr.y0 >= rect_next.y1) {
                DrawBodyConnectionDown(rect_curr, rect_next.y1);
            }
        }
        else if (curr->next_direction_to_tail == right) {
            rect_curr.x0 -= progress_size;
            rect_curr.x1 -= progress_size;
            if (rect_curr.x1 <= rect_next.x0) {
                DrawBodyConnectionRight(rect_curr, rect_next.x0);
            }
        }
        else if (curr->next_direction_to_tail == left) {
            rect_curr.x0 += progress_size;
            rect_curr.x1 += progress_size;
            if (rect_curr.x0 >= rect_next.x1) {
                DrawBodyConnectionLeft(rect_curr, rect_next.x1);
            }
        }
        g_renderer.PushRectangle(rect_curr, body_color, k_z_body);
    }


    // draw remaining body
    curr = next;
    next = next + 1;
    rect_curr = rect_next;
    while (curr != m_body_parts.end()) {
        g_renderer.PushRectangle(rect_curr, body_color, k_z_body);


        float x0_next = TilemapXToWorldX(next->tile_pos.x) + bodypart_offset;
        float y0_next = TilemapYToWorldY(next->tile_pos.y) + bodypart_offset;
        rect_next = {
            x0_next,
            y0_next,
            x0_next + bodypart_size,
            y0_next + bodypart_size
        };


        if (next != m_body_parts.end()) { // don't draw connection to vanishing tail
            Direction next_direction_to_tail = curr->next_direction_to_tail;
            if (next_direction_to_tail == up) {
                DrawBodyConnectionUp(rect_curr, rect_next.y0);
            }
            else if (next_direction_to_tail == down) {
                DrawBodyConnectionDown(rect_curr, rect_next.y1);
            }
            else if (next_direction_to_tail == right) {
                DrawBodyConnectionRight(rect_curr, rect_next.x0);
            }
            else if (next_direction_to_tail == left) {
                DrawBodyConnectionLeft(rect_curr, rect_next.x1);
            }
        }


        curr = next;
        next = next + 1;
        rect_curr = rect_next;
    }


    // draw vanishing tail
    {
        BodyPart tail = m_body_parts.back();
        float tail_x0 = TilemapXToWorldX(tail.tile_pos.x) + bodypart_offset;
        float tail_y0 = TilemapYToWorldY(tail.tile_pos.y) + bodypart_offset;
        float tail_x1 = tail_x0 + bodypart_size;
        float tail_y1 = tail_y0 + bodypart_size;
        float progress_size = (1.0f-m_tile_progress) * k_tile_size;
        Rectangle vanishing_tail_rect;
        if (tail.next_direction_to_tail == up) {
            vanishing_tail_rect.x0 = tail_x0;
            vanishing_tail_rect.y0 = tail_y1;
            vanishing_tail_rect.x1 = tail_x1;
            vanishing_tail_rect.y1 = tail_y1 + progress_size;
        }
        else if (tail.next_direction_to_tail == down) {
            vanishing_tail_rect.x0 = tail_x0;
            vanishing_tail_rect.y0 = tail_y0 - progress_size;
            vanishing_tail_rect.x1 = tail_x1;
            vanishing_tail_rect.y1 = tail_y0;
        }
        else if (tail.next_direction_to_tail == right) {
            vanishing_tail_rect.x0 = tail_x1;
            vanishing_tail_rect.y0 = tail_y0;
            vanishing_tail_rect.x1 = tail_x1 + progress_size;
            vanishing_tail_rect.y1 = tail_y1;
        }
        else if (tail.next_direction_to_tail == left) {
            vanishing_tail_rect.x0 = tail_x0 - progress_size;
            vanishing_tail_rect.y0 = tail_y0;
            vanishing_tail_rect.x1 = tail_x0;
            vanishing_tail_rect.y1 = tail_y1;
        }
        if (tail.next_direction_to_tail != none) {
            g_renderer.PushRectangle(vanishing_tail_rect, k_color_body, k_z_body);
        }
    }
}

void
Fnake::DrawBodyConnectionUp(Rectangle rect_origin, float top_y0)
{
    Rectangle rect = {
        rect_origin.x0,
        rect_origin.y1,
        rect_origin.x1,
        top_y0
    };
    g_renderer.PushRectangle(rect, k_color_body, k_z_body);
}

void
Fnake::DrawBodyConnectionDown(Rectangle rect_origin, float bot_y1)
{
    Rectangle rect = {
        rect_origin.x0,
        bot_y1,
        rect_origin.x1,
        rect_origin.y0
    };
    g_renderer.PushRectangle(rect, k_color_body, k_z_body);
}

void
Fnake::DrawBodyConnectionRight(Rectangle rect_origin, float right_x0)
{
    Rectangle rect = {
        rect_origin.x1,
        rect_origin.y0,
        right_x0,
        rect_origin.y1
    };
    g_renderer.PushRectangle(rect, k_color_body, k_z_body);
}

void
Fnake::DrawBodyConnectionLeft(Rectangle rect_origin, float left_x1)
{
    Rectangle rect = {
        left_x1,
        rect_origin.y0,
        rect_origin.x0,
        rect_origin.y1
    };
    g_renderer.PushRectangle(rect, k_color_body, k_z_body);
}

void
Fnake::DrawFood()
{
    float bodypart_size = 0.8f * k_tile_size;
    float bodypart_offset = (k_tile_size - bodypart_size) / 2;

    Color food_color = {0.5f, 0.0f, 0.0f, 1.0f};
    float food_x = TilemapXToWorldX(m_food_tile_pos.x);
    float food_y = TilemapYToWorldY(m_food_tile_pos.y);

    Rectangle rect = {
        food_x + bodypart_offset,
        food_y + bodypart_offset,
        rect.x0 + bodypart_size,
        rect.y0 + bodypart_size
    };
    g_renderer.PushRectangle(rect, food_color, k_z_food);
}

void
Fnake::DrawScores()
{
    String32Id score_label = MemoryManager::EmplaceString32_Frame(U"Score");
    String32Id score_value = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_score));

    String32Id highscore_label = MemoryManager::EmplaceString32_Frame(U"Highscore");
    String32Id highscore_value = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_highscore));


    float y = k_tilemap_y + k_tiles_y * k_tile_size + 0.75f * k_tile_size;
    float x = k_tilemap_x + 0.5f * k_tile_size;
    V2F32 score_pos = {x , y};
    V2F32 highscore_pos = {x + 0.75f * (k_tiles_x * k_tile_size), y};
    Color anyscore_color {0.9f, 0.9f, 0.9f, 1.0f};

    g_renderer.PushString32(highscore_label, m_font, highscore_pos, anyscore_color, k_z_text);
    highscore_pos.y -= 0.1f;
    g_renderer.PushString32(highscore_value, m_font, highscore_pos, anyscore_color, k_z_text);

    g_renderer.PushString32(score_label, m_font, score_pos, anyscore_color, k_z_text);
    score_pos.y -= 0.1f;
    g_renderer.PushString32(score_value, m_font, score_pos, anyscore_color, k_z_text);
}

float
Fnake::TilemapXToWorldX(int32_t tile_x)
{
    float x = k_tilemap_x + (float)tile_x * k_tile_size;
    return x;
}

float
Fnake::TilemapYToWorldY(int32_t tile_y)
{
    float y = k_tilemap_y + (float)tile_y * k_tile_size;
    return y;
}

