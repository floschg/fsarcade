#include "games/fasteroids/Fasteroids.hpp"
#include "renderer/Renderer.hpp"


static constexpr uint32_t k_z_bg = 1;
static constexpr uint32_t k_z_ship = 2;
static constexpr uint32_t k_z_asteroids = 3;


std::unique_ptr<Game>
Game::CreateFasteroids()
{
    return std::make_unique<Fasteroids>();
}


Spaceship::Spaceship()
{
    float half_w = 0.3f;
    float half_h = 0.3f;
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
Spaceship::Draw()
{
    Color color = {0.4f, 0.4f, 0.4f};
    g_renderer.PushMesh(m_mesh, m_pos, k_z_ship, m_ori, color);
}


void
Fasteroids::Start()
{
    g_renderer.SetCameraSize(4.0f, 3.0f);

    m_ship.m_pos = {2.0f, 2.0f};
    m_ship.m_ori = 1.0f;

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
    AABB bg_aabb = {0.0f, 0.0f, 4.0f, 3.0f};
    Color bg_color = {0.01f, 0.01f, 0.01f};
    g_renderer.PushAABB(bg_aabb, bg_color, k_z_bg);


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

