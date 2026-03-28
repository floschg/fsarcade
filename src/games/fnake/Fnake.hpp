#pragma once

#include "games/Game.hpp"
#include "common/math.hpp"
#include "common/Font.hpp"

#include <random>
#include <deque>


class Fnake : public Game {
public:
    enum Direction : int32_t {
        up,
        down,
        left,
        right,
    };


public:
    Fnake();


private:
    void Start() override;
    void ProcessEvent(SDL_Event& event) override;
    void Update(float dt) override;
    void Draw() override;

    void MaybeMoveBody(float dt_in_seconds);
    void SpawnFood();

    void HandleGameOver();


private:
    static constexpr int32_t k_max_map_width = 14;
    static constexpr int32_t k_max_map_height = 14;
    static constexpr float k_tiles_per_second = 4.0f;
    static constexpr char k_highscore_path[] = "fnake_highscore.txt";

    Font m_font;

    std::mt19937 m_rng;
    std::uniform_int_distribution<int32_t> m_dist;

    Direction m_direction;
    Direction m_last_advanced_direction;

    int32_t m_map_width;
    int32_t m_map_height;

    uint64_t m_body_bitmap[k_max_map_height];
    std::deque<V2I32> m_body_positions;

    V2I32 m_food_position;

    int32_t m_score;
    int32_t m_highscore;
};


