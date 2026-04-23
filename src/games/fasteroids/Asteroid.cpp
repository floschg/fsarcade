#include "games/fasteroids/Asteroid.hpp"
#include "games/fasteroids/Fasteroids.hpp"

#include <cmath>

static constexpr float k_pi = static_cast<float>(std::numbers::pi);
static constexpr float k_pi2 = k_pi*2;

Asteroid::Asteroid()
{
    // temporarily a rectangle
    float half_w = 0.1f;
    float half_h = 0.1f;
    float vertices[] = {
        -half_w, -half_h,
         half_w, -half_h,
        -half_w,  half_h,
         half_w,  half_h
    };
    uint32_t indices[] = {
        0, 1, 2,
        1, 2 ,3
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

    m_pos = {0.5f, 0.5f};
    m_speed = 0.01f;
    m_angle = -k_pi/8;
}

void
Asteroid::Draw()
{
    Color color = {0.5f, 0.3f, 0.6f};
    g_renderer.PushMesh(m_mesh, m_pos, Fasteroids::k_z_asteroids, m_angle, color);
}

void
Asteroid::Update(float dt)
{
    m_pos.x += m_speed * std::cos(m_angle + k_pi/2);
    m_pos.y += m_speed * std::sin(m_angle + k_pi/2);
}

