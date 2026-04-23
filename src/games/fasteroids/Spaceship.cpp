#include "games/fasteroids/Spaceship.hpp"
#include "games/fasteroids/Fasteroids.hpp"

#include <cmath>


static constexpr float k_pi = static_cast<float>(std::numbers::pi);
static constexpr float k_pi2 = k_pi*2;


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

    m_pos = {2.0f, 2.0f};
    m_angle = 0.0f;
}

void
Spaceship::BoostForward()
{
    m_speed_prop = 1.0f;
}

void
Spaceship::RotateClockwise(float angle)
{
    m_angle -= angle;
    while (angle < -k_pi2) {
        m_angle += k_pi2;
    }
}

void
Spaceship::RotateCounterClockwise(float angle)
{
    m_angle += angle;
    while (angle > k_pi2) {
        m_angle -= k_pi2;
    }
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
    uint32_t z = Fasteroids::k_z_spaceship;
    Color color = {0.4f, 0.4f, 0.4f};
    g_renderer.PushMesh(m_mesh, m_pos, z, m_angle, color);
}

Lazer
Spaceship::ShootLazer()
{
    float speed = 1.2f;
    float r = 0.05f;
    Lazer lazer {
        {m_pos.x, m_pos.y, r},
        {speed * std::cos(m_angle + k_pi/2),
         speed * std::sin(m_angle + k_pi/2)}
    };
    return lazer;
}

void
Lazer::Draw()
{
    Color color = {0.8f, 0.8f, 0.8f};
    g_renderer.PushCircle(circle, color, Fasteroids::k_z_lazer);
}

void
Lazer::Update(float dt)
{
    circle.x += velocity.x * dt;
    circle.y += velocity.y * dt;
}

