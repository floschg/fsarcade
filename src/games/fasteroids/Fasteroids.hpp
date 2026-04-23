#pragma once

#include "games/Game.hpp"
#include "games/fasteroids/Spaceship.hpp"
#include "games/fasteroids/Asteroid.hpp"


class Fasteroids : public Game {
public:
    Fasteroids() = default;
    void Start() override;
    void ProcessEvent(SDL_Event& event) override;
    void Update(float dt) override;
    void Draw() override;

public:
    static constexpr uint32_t k_z_spaceship = 1;
    static constexpr uint32_t k_z_asteroids = 2;
    static constexpr uint32_t k_z_lazer = 3;

private:
    Spaceship m_ship;
    std::vector<Asteroid> m_asteroids;
    std::vector<Lazer> m_lazers;
};

