#pragma once

#include "games/Game.hpp"
#include "renderer/Renderer.hpp"

struct Spaceship {
    Spaceship();
    void BoostForward();
    void Update(float dt);
    void Draw();

    V2F32 m_pos;
    float m_angle;
    float m_speed_prop;
    Mesh m_mesh;
};

struct Asteroid {
    V2F32 m_pos;
    float m_ori;
    Mesh m_mesh;
};

class Fasteroids : public Game {
public:
    Fasteroids() = default;
    void Start() override;
    void ProcessEvent(SDL_Event& event) override;
    void Update(float dt) override;
    void Draw() override;

private:
    Spaceship m_ship;
    std::vector<Asteroid> m_asteroids;
};

