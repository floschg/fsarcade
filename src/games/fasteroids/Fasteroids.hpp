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
    static constexpr float k_asteroid_spawn_timer = 0.8f;

private:
    void MaybeSpawnAsteroid(float dt);
    void DespawnDistantLazers();
    void DespawnDistantAsteroids();

private:
    Spaceship m_ship;
    std::vector<Asteroid> m_asteroids;
    std::vector<Lazer> m_lazers;

    // Todo: Maybe use flags or bitset or sth. more memory-efficient later
    bool m_is_boost_forward;
    bool m_is_rotate_clockwise;
    bool m_is_rotate_anticlockwise;

    float m_asteroid_spawn_time_left;
};

