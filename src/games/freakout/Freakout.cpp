#include "games/freakout/Freakout.hpp"
#include "common/math.hpp"
#include "games/Game.hpp"
#include "renderer/Renderer.hpp"

#include <cmath>
#include <memory>

std::unique_ptr<Game>
Game::CreateFreakout()
{
    return std::make_unique<Freakout>();
}

void
Freakout::Start()
{
    float map_cx = (k_map_w / 2);
    m_paddle.x = map_cx - (k_paddle_w / 2);
    m_paddle.dx = 0.0f;


    m_ball.circle.r = 0.05f;
    m_ball.circle.x = map_cx;
    m_ball.circle.y = k_paddle_h + m_ball.circle.r;
    V2F32 ball_velocity = Velocity(0.3f, 0.7f);
    m_ball.dx = ball_velocity.x;
    m_ball.dy = ball_velocity.y;


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

    float brick_y0 = k_map_h - brickmap_h;
    for (uint32_t row = 0; row < k_brick_rows; row++) {
        float brick_x0 = brick_xgap;
        for (uint32_t col = 0; col < k_brick_cols; col++) {
            m_bricks[row][col].x0 = brick_x0;
            m_bricks[row][col].y0 = brick_y0;
            m_bricks[row][col].x1 = brick_x0 + brick_w;
            m_bricks[row][col].y1 = brick_y0 + brick_h;
            brick_x0 += brick_w + brick_xgap;
        }
        brick_y0 += brick_h + brick_ygap;
    }


    static_assert(k_brick_cols <= sizeof(m_brick_bitmap[0])*8);
    uint32_t brick_bitmap_row_init = 0;
    for (size_t x = 0; x < k_brick_cols; x++) {
        brick_bitmap_row_init |= 1<<x;
    }
    for (size_t y = 0; y < k_brick_rows; y++) {
        m_brick_bitmap[y] = brick_bitmap_row_init;
    }


    m_bricks_left = k_brick_rows * k_brick_cols;
    m_score = 0;

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
            m_paddle.dx = k_paddle_speed;
        }
        else if (key == SDLK_LEFT || key == SDLK_A) {
            m_paddle.dx = -k_paddle_speed;
        }
        else {
        }
    } break;

    case SDL_EVENT_KEY_UP: {
        auto key = event.key.key;
        if (key == SDLK_RIGHT || key == SDLK_D) {
            if (m_paddle.dx >= 0.0f) {
                m_paddle.dx = 0.0f;
            }
        }
        else if (key == SDLK_LEFT || key == SDLK_A) {
            if (m_paddle.dx <= 0.0f) {
                m_paddle.dx = 0.0f;
            }
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

enum CollisionSource {up, down, right, left};
static CollisionSource
FindCollisionSource(AABB aabb, Circle circle)
{
    float closest_x = std::max(aabb.x0, std::min(circle.x, aabb.x1));
    float closest_y = std::max(aabb.y0, std::min(circle.y, aabb.y1));

    float dx = circle.x - closest_x;
    float dy = circle.y - closest_y;

    if (std::abs(dx) > std::abs(dy)) {
        return (dx > 0.0f) ? right : left;
    }
    else {
        return (dy > 0.0f) ? up : down;
    }
}

void
Freakout::MoveBall(float dt)
{
    m_ball.circle.x = m_ball.circle.x + m_ball.dx * dt;
    m_ball.circle.y = m_ball.circle.y + m_ball.dy * dt;

    // collision walls
    if (m_ball.circle.x - m_ball.circle.r <= 0.0f) {
        m_ball.circle.x = m_ball.circle.r;
        m_ball.dx = std::abs(m_ball.dx);
    }
    if (m_ball.circle.x + m_ball.circle.r >= k_map_w) {
        m_ball.circle.x = k_map_w - m_ball.circle.r;
        m_ball.dx = -std::abs(m_ball.dx);
    }
    if (m_ball.circle.y <= 0.0f) {
        m_game_status = game_over;
    }
    if (m_ball.circle.y + m_ball.circle.r >= k_map_h) {
        m_ball.circle.y = k_map_h - m_ball.circle.r;
        m_ball.dy = -std::abs(m_ball.dy);
    }

    // collision paddle
    AABB paddle_aabb = {
        m_paddle.x,
        0.0f,
        m_paddle.x + k_paddle_w,
        k_paddle_h
    };
    if (Intersect_AABB_Circle(paddle_aabb, m_ball.circle)) {
        CollisionSource collision_source = FindCollisionSource(paddle_aabb, m_ball.circle);
        if (collision_source == up) {
            float paddle_half_w = k_paddle_w / 2.0f;
            float paddle_cx = m_paddle.x + paddle_half_w;
            float dist_cx = m_ball.circle.x - paddle_cx;
            float dist_cx_prop = dist_cx / paddle_half_w;

            float dx_prop = 0.6f * dist_cx_prop;
            float dy_prop = 1.0f - std::abs(dx_prop);

            V2F32 velocity = Velocity(dx_prop, dy_prop);

            m_ball.circle.y = paddle_aabb.y1 + m_ball.circle.r;
            m_ball.dx = velocity.x;
            m_ball.dy = velocity.y;
        }
        else if (collision_source == right) {
            m_ball.circle.x = paddle_aabb.x1 + m_ball.circle.r;
            m_ball.dx = -m_ball.dx;
        }
        else if (collision_source == left) {
            m_ball.circle.x = paddle_aabb.x0 - m_ball.circle.r;
            m_ball.dx = -m_ball.dx;
        }
    }

    // collision bricks
    for (size_t y = 0; y < k_brick_rows; y++) {
        for (size_t x = 0; x < k_brick_cols; x++) {
            bool is_alive = m_brick_bitmap[y] & (1<<x);
            bool is_collision = is_alive && Intersect_AABB_Circle(m_bricks[y][x], m_ball.circle);
            if (is_collision) {
                AABB brick = m_bricks[y][x];
                CollisionSource collision_source = FindCollisionSource(brick, m_ball.circle);
                if (collision_source == up) {
                    m_ball.circle.y = brick.y1 + m_ball.circle.r;
                    m_ball.dy = -m_ball.dy;
                }
                else if (collision_source == down) {
                    m_ball.circle.y = brick.y0 - m_ball.circle.r;
                    m_ball.dy = -m_ball.dy;
                }
                else if (collision_source == right) {
                    m_ball.circle.x = brick.x1 + m_ball.circle.r;
                    m_ball.dx = -m_ball.dx;
                }
                else {
                    m_ball.circle.x = brick.x0 - m_ball.circle.r;
                    m_ball.dx = -m_ball.dx;
                }

                m_brick_bitmap[y] &= ~(1u<<x);
                m_score += k_scores[y];
                m_bricks_left -= 1;
            }
        }
    }
    if (m_bricks_left == 0) {
        m_game_status = game_over;
    }
}

V2F32
Freakout::Velocity(float dx_prop, float dy_prop)
{
    float length = std::sqrt(dx_prop*dx_prop + dy_prop*dy_prop);
    float dx_normalized = dx_prop / length;
    float dy_normalized = dy_prop / length;

    V2F32 velocity = {
        k_ball_speed * dx_normalized,
        k_ball_speed * dy_normalized
    };
    return velocity;
}

void
Freakout::Draw()
{
    Color ball_color = {0.6f, 0.6f, 0.6f, 1.0f};
    g_renderer.PushCircle(m_ball.circle, ball_color, k_z_ball);


    Color paddle_color = {0.3f, 0.3f, 0.6f, 1.0f};
    AABB paddle_aabb = {
        m_paddle.x,
        0.0f,
        m_paddle.x + k_paddle_w,
        k_paddle_h
    };
    g_renderer.PushAABB(paddle_aabb, paddle_color, k_z_paddle);


    for (uint32_t y = 0; y < k_brick_rows; y++) {
        for (uint32_t x = 0; x < k_brick_cols; x++) {
            if (m_brick_bitmap[y] & (1 << x)) {
                g_renderer.PushAABB(m_bricks[y][x], k_brick_colors[y], k_z_brick);
            }
        }
    }
}

void
Freakout::DrawGameOverMenu()
{
    ImGui::Begin("FreakoutGameOverMenu", nullptr, k_imgui_window_flags_menu);
    if (m_bricks_left == 0) {
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


