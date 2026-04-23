#pragma once

#include "common/math.hpp"
#include "renderer/Renderer.hpp"

struct Lazer {
    Circle circle;
    V2F32 velocity;
    void Draw();
    void Update(float dt);
};

class Spaceship {
public:
    Spaceship();
    void BoostForward();
    void RotateClockwise(float angle);
    void RotateCounterClockwise(float angle);
    Lazer ShootLazer();

    void Update(float dt);
    void Draw();

private:
    V2F32 m_pos;
    float m_angle;
    float m_speed_prop;
    Mesh m_mesh;
};

