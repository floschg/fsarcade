#include "games/fasteroids/Fasteroids.hpp"
#include "renderer/Renderer.hpp"

std::unique_ptr<Game>
Game::CreateFasteroids()
{
    return std::make_unique<Fasteroids>();
}

void
Fasteroids::Start()
{
    g_renderer.SetCameraSize(4.0f, 3.0f);
    m_ship.m_pos = {2.0f, 2.0f};
    m_ship.m_ori = {1.0f, 0.0f};

    m_asteroids.clear();
    m_asteroids.emplace_back(V2F32{1.0f, 1.0f}, V2F32{1.0f, 0.0f});
    m_game_status = game_resume;
}

void
Fasteroids::ProcessEvent(SDL_Event& event)
{
}

void
Fasteroids::Update(float dt)
{
}

void
Fasteroids::Draw()
{
    uint32_t z_bg = 1;
    uint32_t z_ship = 2;
    uint32_t z_asteroid = 3;


    AABB bg_aabb = {0.0f, 0.0f, 4.0f, 3.0f};
    Color bg_color = {0.01f, 0.01f, 0.01f};
    g_renderer.PushAABB(bg_aabb, bg_color, z_bg);


    Color ship_color = {0.4f, 0.4f, 0.4f};
    AABB ship_aabb = {
        m_ship.m_pos.x,
        m_ship.m_pos.y,
        m_ship.m_pos.x + 0.4f,
        m_ship.m_pos.y + 0.2f,
    };
    g_renderer.PushAABB(ship_aabb, ship_color, z_ship);


    Color asteroid_color = {0.5f, 0.3f, 0.6f};
    for (Asteroid& asteroid : m_asteroids) {
        AABB asteroid_aabb = {
            asteroid.m_pos.x,
            asteroid.m_pos.y,
            asteroid.m_pos.x + 0.2f,
            asteroid.m_pos.y + 0.2f,
        };
        g_renderer.PushAABB(asteroid_aabb, asteroid_color, z_asteroid);
    }
}

