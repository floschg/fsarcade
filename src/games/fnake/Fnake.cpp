#include "games/fnake/Fnake.hpp"
#include "games/Game.hpp"
#include "renderer/Renderer.hpp"
#include "common/MemoryManager.hpp"
#include "common/math.hpp"

#include <imgui.h>

#include <cstdint>
#include <fstream>


Fnake::Fnake()
    : m_font {k_dejavu_sans_mono_filepath, 22}
    , m_rng {std::random_device{}()}
{
    static_assert(k_max_map_width <= sizeof(m_body_bitmap[0])*8);
    static_assert(k_max_map_height <= sizeof(m_body_bitmap[0])*8);

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
    m_dt_remaining_seconds = 0.0f;
    m_tlast_milliseconds = SDL_GetTicks();

    m_direction = right;
    m_last_advanced_direction = right;

    m_map_width = 12;
    m_map_height = 10;
    assert(m_map_width <= k_max_map_width);
    assert(m_map_height <= k_max_map_height);

    memset(m_body_bitmap, 0, sizeof(m_body_bitmap));

    int32_t head_x = m_map_width / 2;
    int32_t head_y = m_map_height / 2;
    m_body_positions.clear();
    m_body_positions.emplace_front(head_x-1, head_y);
    m_body_positions.emplace_front(head_x, head_y);

    SpawnFood();

    m_score = 0;
}

void
Fnake::Update(float dt)
{
    MaybeMoveBody(dt);
}

void
Fnake::MaybeMoveBody(float dt)
{
    float seconds_per_tile = 1.0f / k_tiles_per_second;
    while (dt > seconds_per_tile) {
        V2I32 head_pos = m_body_positions.front();
        V2I32 tail_pos = m_body_positions.back();

        // find next head_pos
        if (m_direction == up) {
            head_pos.y += 1;
        }
        else if (m_direction == down) {
            head_pos.y -= 1;
        }
        else if (m_direction == right) {
            head_pos.x += 1;
        }
        else if (m_direction == left) {
            head_pos.x -= 1;
        }
        if ((head_pos.x < 0 || head_pos.x >= m_map_width) ||
            (head_pos.y < 0 || head_pos.y >= m_map_height))
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
        m_body_positions.emplace_front(head_pos);

        if (m_body_positions.front() == m_food_position) {
            // eat food
            m_score += 1;
            SpawnFood();
        }
        else {
            // advance tail
            m_body_bitmap[tail_pos.y] &= (uint32_t)~(1 << tail_pos.x);
            m_body_positions.pop_back();
        }


        m_last_advanced_direction = m_direction;
        dt -= seconds_per_tile;
    }

    m_dt_remaining_seconds = dt;
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
            if (m_last_advanced_direction == right ||
                m_last_advanced_direction == left)
            {
                m_direction = up;
            }
        }
        else if (event.key.key == SDLK_DOWN) {
            if (m_last_advanced_direction == right ||
                m_last_advanced_direction == left)
            {
                m_direction = down;
            }
        }
        else if (event.key.key == SDLK_RIGHT) {
            if (m_last_advanced_direction == up ||
                m_last_advanced_direction == down)
            {
                m_direction = right;
            }
        }
        else if (event.key.key == SDLK_LEFT) {
            if (m_last_advanced_direction == up ||
                m_last_advanced_direction == down)
            {
                m_direction = left;
            }
        }
    }

    default:;
    }
}

void
Fnake::SpawnFood()
{
    int32_t bit0_counts[k_max_map_height];
    int32_t bit0_count_total = 0;

    // count bits
    for (int32_t y = 0; y < m_map_height; y++) {
        int32_t bit1_count = 0;

        uint64_t bitmap_row = m_body_bitmap[y];
        while (bitmap_row != 0) {
            bitmap_row = bitmap_row & (bitmap_row - 1);
            bit1_count += 1;
        }

        int32_t bit0_count = m_map_width - bit1_count;
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
    for (int32_t y = 0; y < m_map_height; y++) {
        if (bit0_index < bit0_counts[y]) {
            bit0_y = y;
            break;
        }
        bit0_index -= bit0_counts[y];
    }

    // find x
    uint64_t bitmap_row_not = ~m_body_bitmap[bit0_y];
    for (int32_t x = 0; x < m_map_width; x++) {
        if (bitmap_row_not & 1) {
            if (bit0_index == 0) {
                bit0_x = x;
                break;
            }
            bit0_index--;
        }
        bitmap_row_not >>= 1;
    }

    m_food_position = {bit0_x, bit0_y};
}

void
Fnake::Draw()
{
    float world_width = 4.0f;
    float world_height = 3.0f;
    float tile_size = (world_width / 2) / k_max_map_width;

    float bodypart_size = 0.8f * tile_size;
    float bodypart_offset = (tile_size - bodypart_size) / 2;

    float map_width = tile_size * (float)m_map_width;
    float map_height = tile_size * (float)m_map_height;
    float map_x = (world_width - map_width) / 2;
    float map_y = (world_height - map_height) / 2;

    uint32_t z_bg = z_layer1;
    uint32_t z_food = z_layer2;
    uint32_t z_fnake = z_layer3;

    Color color_fnake = {0.0f, 0.5f, 0.0f, 1.0f};
    Color color_food  = {0.5f, 0.0f, 0.0f, 1.0f};
    Color color_bg    = {0.0f, 0.0f, 0.0f, 1.0f};


    // draw map background
    Rectangle map_world_rect = {
        map_x,
        map_y,
        map_x + map_width,
        map_y + map_height
    };
    g_renderer.PushRectangle(map_world_rect, color_bg, z_bg);


    // draw body
    for (auto it = m_body_positions.begin(); it != m_body_positions.end(); it++) {
        float xoff = (float)it->x * tile_size + bodypart_offset;
        float yoff = (float)it->y * tile_size + bodypart_offset;

        float x = map_x + xoff;
        float y = map_y + yoff;

        Rectangle rect = {
            x,
            y,
            x + bodypart_size,
            y + bodypart_size
        };

        g_renderer.PushRectangle(rect, color_fnake, z_fnake);
    }


    // draw food
    float food_x = map_x + (float)m_food_position.x * tile_size + bodypart_offset;
    float food_y = map_y + (float)m_food_position.y * tile_size + bodypart_offset;
    Rectangle rect = {
        food_x,
        food_y,
        food_x + bodypart_size,
        food_y + bodypart_size
    };
    g_renderer.PushRectangle(rect, color_food, z_food);


    // draw scores
    String32Id score_label = MemoryManager::EmplaceString32_Frame(U"Score");
    String32Id score_value = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_score));

    String32Id highscore_label = MemoryManager::EmplaceString32_Frame(U"Highscore");
    String32Id highscore_value = MemoryManager::EmplaceString32_Frame(int32_to_u32string(m_highscore));


    V2F32 score_pos = {1.3f, 2.5f};
    V2F32 highscore_pos = {2.3f, 2.5f};
    Color anyscore_color {0.9f, 0.9f, 0.9f, 1.0f};

    g_renderer.PushString32(highscore_label, m_font, highscore_pos, anyscore_color, z_text);
    highscore_pos.y -= 0.1f;
    g_renderer.PushString32(highscore_value, m_font, highscore_pos, anyscore_color, z_text);

    g_renderer.PushString32(score_label, m_font, score_pos, anyscore_color, z_text);
    score_pos.y -= 0.1f;
    g_renderer.PushString32(score_value, m_font, score_pos, anyscore_color, z_text);
}

