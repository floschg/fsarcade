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

    static constexpr float k_map_w = 4.0f;
    static constexpr float k_map_h = 3.0f;

    static constexpr uint32_t k_brick_rows = 8;
    static constexpr uint32_t k_brick_cols = 14;

    static constexpr float k_ball_speed = 2.0f;

    static constexpr float k_paddle_w = 0.6f;
    static constexpr float k_paddle_h = 0.1f;
    static constexpr float k_paddle_speed = 1.0f;


private:
    void Start() override;
    void ProcessEvent(SDL_Event& event) override;
    void Update(float dt) override;
    void Draw() override;

private:
    void MovePaddle(float dt);
    void MoveBall(float dt);

private:
    Rectangle m_bricks[k_brick_rows][k_brick_cols];
    Paddle m_paddle;
    Ball m_ball;
};

