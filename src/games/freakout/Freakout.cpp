#include "games/freakout/Freakout.hpp"
#include "common/math.hpp"
#include "games/Game.hpp"
#include "common/shapes.hpp"
#include "renderer/Renderer.hpp"

#include <cmath>


void
Freakout::Start()
{
    float xmid = (3.0f / 2);
    m_paddle.x = xmid - (k_paddle_w / 2);
    m_paddle.dx = 0.0f;


    m_ball.circle.r = 0.05f;
    m_ball.circle.x = xmid;
    m_ball.circle.y = k_paddle_h + m_ball.circle.r;
    m_ball.dx = 0.0f;
    m_ball.dy = k_ball_speed;


    float brickmap_w = 1.00f * k_map_w;
    float brickmap_h = 0.25f * k_map_h;

    float brick_w_sum = 0.80f * brickmap_w;
    float brick_h_sum = 0.70f * brickmap_h;
    float brick_xgap_sum = brickmap_w - brick_w_sum;
    float brick_ygap_sum = brickmap_h - brick_h_sum;

    float brick_w = brick_w_sum / k_brick_cols;
    float brick_h = brick_h_sum / k_brick_rows;
    float brick_xgap = brick_xgap_sum / (k_brick_cols+1);
    float brick_ygap = brick_ygap_sum / (k_brick_rows+1);

    float y = k_map_h - brickmap_h;
    for (uint32_t row = 0; row < k_brick_rows; row++) {
        float x = brick_xgap;
        for (uint32_t col = 0; col < k_brick_cols; col++) {
            m_bricks[row][col].x0 = x;
            m_bricks[row][col].y0 = y;
            m_bricks[row][col].x1 = x + brick_w;
            m_bricks[row][col].y1 = y + brick_h;
            x += brick_w + brick_xgap;
        }
        y += brick_h + brick_ygap;
    }


    m_game_status = game_resume;
}

void
Freakout::ProcessEvent(SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        auto key = event.key.key;
        if (key == SDLK_ESCAPE) {
            m_game_status = game_pause;
        }
        else if (key == SDLK_RIGHT || key == SDLK_D) {
            m_paddle.dx = 1.0f;
        }
        else if (key == SDLK_LEFT || key == SDLK_A) {
            m_paddle.dx = -1.0f;
        }
        else {
        }
    } break;

    case SDL_EVENT_KEY_UP: {
        auto key = event.key.key;
        if (key == SDLK_RIGHT || key == SDLK_D) {
            m_paddle.dx = 0.0f;
        }
        else if (key == SDLK_LEFT || key == SDLK_A) {
            m_paddle.dx = 0.0f;
        }
    } break;

    default:;
    }
}

void
Freakout::Update(float dt)
{
    MoveBall(dt);
    MovePaddle(dt);
}

void
Freakout::MovePaddle(float dt)
{
    float x = m_paddle.x + m_paddle.dx * dt;
    if (x + k_paddle_w >= k_map_w) {
        x = k_map_w - k_paddle_w;
    }
    if (x <= 0.0f) {
        x = 0.0f;
    }
    m_paddle.x = x;
}

void
Freakout::MoveBall(float dt)
{
    float x = m_ball.circle.x + m_ball.dx * dt;
    float y = m_ball.circle.y + m_ball.dy * dt;
    m_ball.circle.x = x;
    m_ball.circle.y = y;

    // collision walls
    if (m_ball.circle.x <= 0.0f) {
        m_ball.dx = std::abs(m_ball.dx);
    }
    if (m_ball.circle.x + m_ball.circle.r >= k_map_w) {
        m_ball.dx = -std::abs(m_ball.dx);
    }
    if (m_ball.circle.y <= 0.0f) {
        m_game_status = game_over;
    }
    if (m_ball.circle.y + m_ball.circle.r >= k_map_h) {
        m_ball.dy = -std::abs(m_ball.dy);
    }

    // collision paddle
    // Todo: Handle collisions on the side of the paddle near the bottom.
    //       Those should end up in game over somehow.
    Rectangle paddle_rect = {
        m_paddle.x,
        0.0f,
        m_paddle.x + k_paddle_w,
        k_paddle_h
    };
    if (Intersect_AABB_Circle(paddle_rect, m_ball.circle)) {
        // Todo: find a better name than 'percent' (which represents [0.0, 1.0] here instead of the expected [0, 100]).
        float rect_half_width = (paddle_rect.x1 - paddle_rect.x0) / 2;
        float rect_center_x = paddle_rect.x0 + rect_half_width;
        float dist_x = m_ball.circle.x - rect_center_x;
        float dist_x_percent = dist_x / rect_half_width;

        float max_dx_percent = 0.7f;
        float dx_percent = dist_x_percent * max_dx_percent;
        float dy_percent = 1.0f - dx_percent;

        float length = std::sqrt(dx_percent*dx_percent + dy_percent*dy_percent);
        float dx = k_ball_speed * (dx_percent / length);
        float dy = k_ball_speed * (dy_percent / length);

        m_ball.circle.y += paddle_rect.y1 -(m_ball.circle.y - m_ball.circle.r);
        m_ball.dx = dx;
        m_ball.dy = dy;
    }
}

void
Freakout::Draw()
{
    // Todo: draw a circle
    Rectangle ball_rectangle = {
        m_ball.circle.x - m_ball.circle.r,
        m_ball.circle.y - m_ball.circle.r,
        m_ball.circle.x + m_ball.circle.r,
        m_ball.circle.y + m_ball.circle.r
    };
    Color ball_color = {0.3f, 0.5f, 0.3f, 1.0f};
    g_renderer.PushRectangle(ball_rectangle, ball_color, z_layer2);


    Color paddle_color = {0.6f, 0.3f, 0.3f, 1.0f};
    Rectangle paddle_rect = {
        m_paddle.x,
        0.0f,
        m_paddle.x + k_paddle_w,
        k_paddle_h
    };
    g_renderer.PushRectangle(paddle_rect, paddle_color, z_layer1);


    Color brick_color = {0.5f, 0.3f, 0.3f, 1.0f};
    for (uint32_t y = 0; y < k_brick_rows; y++) {
        for (uint32_t x = 0; x < k_brick_cols; x++) {
            g_renderer.PushRectangle(m_bricks[y][x], brick_color, z_layer1);
        }
    }
}

