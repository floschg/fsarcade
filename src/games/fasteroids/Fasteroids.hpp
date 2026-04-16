#pragma once

#include "games/Game.hpp"

struct Spaceship {
    V2F32 m_pos;
    V2F32 m_ori;
};

struct Asteroid {
    V2F32 m_pos;
    V2F32 m_ori;
};

class Fasteroids : public Game {
public:
    void Start() override;
    void ProcessEvent(SDL_Event& event) override;
    void Update(float dt) override;
    void Draw() override;

private:
    Spaceship m_ship;
    std::vector<Asteroid> m_asteroids;
};

