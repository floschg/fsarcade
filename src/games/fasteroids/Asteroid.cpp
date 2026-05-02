#include "games/fasteroids/Asteroid.hpp"
#include "games/fasteroids/Fasteroids.hpp"

#include <cmath>
#include <random>


Asteroid::Asteroid()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());


    float half_w = 0.2f;
    float half_h = 0.2f;

    float vertices[] = {
        // center
        0.0f, 0.0f,

        // bot (right -> left)
         half_w,  -half_h,
        0.0f,    -half_h,
        -half_w, -half_h,

        // mid left
        -half_w, 0.0f,
    };
    uint32_t indices[] = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
    };

    size_t vertex_count = sizeof(vertices)/sizeof(vertices[0]);
    m_mesh.m_vertices.reserve(vertex_count);
    for (size_t i = 0; i < vertex_count; i++) {
        m_mesh.m_vertices.push_back(vertices[i]);
    }

    size_t index_count = sizeof(indices)/sizeof(indices[0]);
    m_mesh.m_indices.reserve(index_count);
    for (size_t i = 0; i < index_count; i++) {
        m_mesh.m_indices.push_back(indices[i]);
    }


    float chaos = 10.0f; // higher value means asteroids move more towards world's center
    std::uniform_real_distribution<> angle_spawn_dist(0.0f, k_pi2);
    std::uniform_real_distribution<> angle_spin_dist(-1.0f, 1.0f);
    std::uniform_real_distribution<> angle_chaos_dist(-k_pi/chaos, k_pi/chaos);

    float angle_spawn = (float)angle_spawn_dist(gen);
    float angle_spin = (float)angle_spin_dist(gen);
    float angle_chaos = (float)angle_chaos_dist(gen);

    float angle_move = angle_spawn + k_pi + angle_chaos;

    float world_cx = 4.0f / 2.0f;
    float world_cy = 3.0f / 2.0f;
    float radius = 3.0f; // = sqrt(a^2 + b^2) + margin for a=2,b=1.5 (world_{w,h} div 2)
    float x = radius * std::cosf(angle_spawn) + world_cx;
    float y = radius * std::sinf(angle_spawn) + world_cy;

    m_pos = {x, y};
    m_angle_ori = 0.0f;
    m_angle_spin = angle_spin;
    m_angle_move = angle_move;
    m_speed = 0.01f;
}

void
Asteroid::Draw()
{
    Color color = {0.5f, 0.3f, 0.6f};
    g_renderer.PushMesh(m_mesh, m_pos, Fasteroids::k_z_asteroids, m_angle_ori, color);
}

void
Asteroid::Update(float dt)
{
    m_pos.x += m_speed * std::cos(m_angle_move);
    m_pos.y += m_speed * std::sin(m_angle_move);
    m_angle_ori += m_angle_spin * dt;
}

