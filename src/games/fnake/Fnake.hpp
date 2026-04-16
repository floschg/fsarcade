#pragma once

#include "games/Game.hpp"
#include "common/math.hpp"
#include "common/Font.hpp"

#include <random>
#include <deque>


class Fnake : public Game {
public:
    Fnake();

    enum Direction : int32_t {
        none = 0,
        whatever = 0,
        up = 1,
        down = -1,
        right = 2 ,
        left = -2,
    };

    struct BodyPart {
        V2I32 tile_pos;
        Direction next_direction_to_head;
        Direction next_direction_to_tail;
    };


private:
    void Start() override;
    void ProcessEvent(SDL_Event& event) override;
    void Update(float dt) override;
    void Draw() override;

    void MoveBody(float dt_in_seconds);
    void SpawnFood();
    void HandleGameOver();

    void DrawBody();
    void DrawBodyConnectionUp(AABB aabb_origin, float top_y0);
    void DrawBodyConnectionDown(AABB aabb_origin, float bot_y1);
    void DrawBodyConnectionRight(AABB aabb_origin, float right_x0);
    void DrawBodyConnectionLeft(AABB aabb_origin, float left_x1);
    void DrawFood();
    void DrawScores();

    float TilemapXToWorldX(int32_t tile_x);
    float TilemapYToWorldY(int32_t tile_y);


private:
    static constexpr float k_world_w = 4.0f;
    static constexpr float k_world_h = 3.0f;

    static constexpr int32_t k_tiles_x = 12;
    static constexpr int32_t k_tiles_y = 12;
    static constexpr float k_tile_size = (k_world_w / 2) / k_tiles_x;
    static constexpr float k_tilemap_x = (k_world_w - (k_tiles_x * k_tile_size)) / 2;
    static constexpr float k_tilemap_y = (k_world_h - (k_tiles_y * k_tile_size)) / 2;

    static constexpr Color k_color_body = {0.0f, 0.5f, 0.0f, 1.0f};

    static constexpr uint32_t k_z_tilemap = 0;
    static constexpr uint32_t k_z_food = 1;
    static constexpr uint32_t k_z_body = 2;
    static constexpr uint32_t k_z_text = 3;

    static constexpr float k_tiles_per_second = 4.0f;

    static constexpr char k_highscore_path[] = "fnake_highscore.txt";


private:
    Font m_font;

    std::mt19937 m_rng;
    std::uniform_int_distribution<int32_t> m_dist;

    Direction m_curr_direction;
    Direction m_next_direction;
    float m_tile_progress;

    uint64_t m_body_bitmap[k_tiles_y];
    std::deque<BodyPart> m_body_parts;
    V2I32 m_food_tile_pos;

    int32_t m_score;
    int32_t m_highscore;
};


