#pragma once

#include "games/Game.hpp"
#include "common/shapes.hpp"


class Freakout : public Game {
    struct Paddle {
        float x;
        float dx;
    };

    struct Ball {
        Circle circle;
        float dx;
        float dy;
    };

    static constexpr uint32_t k_z_bg = 0;
    static constexpr uint32_t k_z_paddle = 1;
    static constexpr uint32_t k_z_brick = 1;
    static constexpr uint32_t k_z_ball = 2;

    static constexpr float k_map_w = 4.0f;
    static constexpr float k_map_h = 3.0f;

    static constexpr uint32_t k_brick_rows = 8;
    static constexpr uint32_t k_brick_cols = 14;

    static constexpr float k_ball_speed = 2.0f;

    static constexpr float k_paddle_w = 0.5f;
    static constexpr float k_paddle_h = 0.1f;
    static constexpr float k_paddle_speed = 2.0f;

    static constexpr uint32_t k_scores[] = {1,1,3,3,5,5,7,7};
    static constexpr Color k_brick_colors[] = {
        // yellow
        {0.8f, 0.8f, 0.1f},
        {0.8f, 0.8f, 0.1f},
        // green
        {0.0f, 0.5f, 0.1f},
        {0.0f, 0.5f, 0.1f},
        // orange
        {0.8f, 0.5f, 0.0f},
        {0.8f, 0.5f, 0.0f},
        // red
        {0.6f, 0.03f, 0.0f},
        {0.6f, 0.03f, 0.0f}
    };


private:
    void Start() override;
    void ProcessEvent(SDL_Event& event) override;
    void Update(float dt) override;
    void Draw() override;

    void DrawGameOverMenu() override;


private:
    void MovePaddle(float dt);
    void MoveBall(float dt);

private:
    uint32_t m_brick_bitmap[k_brick_cols];
    Rectangle m_bricks[k_brick_rows][k_brick_cols];
    Paddle m_paddle;
    Ball m_ball;

    uint32_t m_bricks_left;
    uint32_t m_score;
};

