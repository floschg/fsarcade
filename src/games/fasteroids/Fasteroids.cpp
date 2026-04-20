#include "games/fasteroids/Fasteroids.hpp"
#include "renderer/Renderer.hpp"

#include <cmath>


static constexpr uint32_t k_z_bg = 1;
static constexpr uint32_t k_z_ship = 2;
static constexpr uint32_t k_z_asteroids = 3;
static constexpr float k_pi = static_cast<float>(std::numbers::pi);
static constexpr float k_pi2 = k_pi*2;


std::unique_ptr<Game>
Game::CreateFasteroids()
{
    return std::make_unique<Fasteroids>();
}


Spaceship::Spaceship()
{
    float half_w = 0.1f;
    float half_h = 0.2f;
    float vertices[] = {
        -half_w, -half_h,
         half_w, -half_h,
        0.0f,     half_h
    };

    size_t vertex_count = sizeof(vertices)/sizeof(vertices[0]);
    m_mesh.m_vertices.reserve(vertex_count);
    for (size_t i = 0; i < vertex_count; i++) {
        m_mesh.m_vertices.push_back(vertices[i]);
    }

    uint32_t indices[] = {
        0, 1, 2
    };
    size_t index_count = sizeof(indices)/sizeof(indices[0]);
    m_mesh.m_indices.reserve(index_count);
    for (size_t i = 0; i < index_count; i++) {
        m_mesh.m_indices.push_back(indices[i]);
    }
}

void
Spaceship::BoostForward()
{
    m_speed_prop = 1.0f;
}

void
Spaceship::Update(float dt)
{
    float speed = 0.8f;
    if (m_speed_prop >= 0.0f) {
        m_pos.x += m_speed_prop * speed * dt * std::cosf(m_angle + k_pi/2);
        m_pos.y += m_speed_prop * speed * dt * std::sinf(m_angle + k_pi/2);
        m_speed_prop -= 0.005f;
    }

    if (m_speed_prop < 0.0f) {
        m_speed_prop = 0.0f;
    }
}

void
Spaceship::Draw()
{
    Color color = {0.4f, 0.4f, 0.4f};
    g_renderer.PushMesh(m_mesh, m_pos, k_z_ship, m_angle, color);
}


void
Fasteroids::Start()
{
    g_renderer.SetCameraSize(4.0f, 3.0f);

    m_ship.m_pos = {2.0f, 2.0f};
    m_ship.m_angle = 0.0f;

    m_game_status = game_resume;
}

void
Fasteroids::ProcessEvent(SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        SDL_Keycode keycode = event.key.key;
        if (keycode == SDLK_LEFT) {
            m_ship.m_angle += k_pi/32;
            while (m_ship.m_angle > k_pi2) {
                m_ship.m_angle -= k_pi2;
            }
        }
        else if (keycode == SDLK_RIGHT) {
            m_ship.m_angle -= k_pi/32;
            while (m_ship.m_angle < -k_pi2) {
                m_ship.m_angle += k_pi2;
            }
        }
        else if (keycode == SDLK_UP) {
            m_ship.BoostForward();
        }
        else if (keycode == SDLK_SPACE) {
            // Todo: shoot laser
        }
    }
    }
}

void
Fasteroids::Update(float dt)
{
    m_ship.Update(dt);
}

void
Fasteroids::Draw()
{
    Color bg_color = {0.01f, 0.01f, 0.01f};
    g_renderer.SetClearColor(bg_color);


    m_ship.Draw();


    Color asteroid_color = {0.5f, 0.3f, 0.6f};
    for (Asteroid& asteroid : m_asteroids) {
        AABB asteroid_aabb = {
            asteroid.m_pos.x,
            asteroid.m_pos.y,
            asteroid.m_pos.x + 0.2f,
            asteroid.m_pos.y + 0.2f,
        };
        g_renderer.PushAABB(asteroid_aabb, asteroid_color, k_z_asteroids);
    }
}

