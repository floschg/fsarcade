#include "games/fasteroids/Fasteroids.hpp"
#include "common/math.hpp"
#include "renderer/Renderer.hpp"


std::unique_ptr<Game>
Game::CreateFasteroids()
{
    return std::make_unique<Fasteroids>();
}

void
Fasteroids::Start()
{
    g_renderer.SetCameraSize(4.0f, 3.0f);

    m_ship.Reset();

    m_lazers.clear();

    m_asteroids.clear();
    m_asteroids.push_back(Asteroid());

    m_is_boost_forward = false;
    m_is_rotate_clockwise = false;
    m_is_rotate_anticlockwise = false;

    m_asteroid_spawn_time_left = k_asteroid_spawn_timer;

    m_game_status = game_resume;
}

void
Fasteroids::ProcessEvent(SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        SDL_Keycode keycode = event.key.key;
        if (keycode == SDLK_LEFT) {
            m_is_rotate_anticlockwise = true;
        }
        else if (keycode == SDLK_RIGHT) {
            m_is_rotate_clockwise = true;
        }
        else if (keycode == SDLK_UP) {
            m_is_boost_forward = true;
        }
        else if (keycode == SDLK_SPACE) {
            if (!event.key.repeat) {
                m_lazers.emplace_back(m_ship.ShootLazer());
            }
        }
    } break;
    case SDL_EVENT_KEY_UP: {
        SDL_Keycode keycode = event.key.key;
        if (keycode == SDLK_LEFT) {
            m_is_rotate_anticlockwise = false;
        }
        else if (keycode == SDLK_RIGHT) {
            m_is_rotate_clockwise = false;
        }
        else if (keycode == SDLK_UP) {
            m_is_boost_forward = false;
        }
    } break;
    }
}

void
Fasteroids::Update(float dt)
{
    if (m_is_boost_forward) {
        m_ship.StartBoost();
    }
    if (m_is_rotate_clockwise) {
        m_ship.RotateClockwise(dt);
    }
    if (m_is_rotate_anticlockwise) {
        m_ship.RotateAntiClockwise(dt);
    }
    m_ship.MoveForward(dt);


    for (Lazer& lazer : m_lazers) {
        lazer.Update(dt);
    }
    DespawnDistantLazers();


    for (Asteroid& asteroid : m_asteroids) {
        asteroid.Update(dt);
    }
    DespawnDistantAsteroids();


    MaybeSpawnAsteroid(dt);
}

void
Fasteroids::MaybeSpawnAsteroid(float dt)
{
    float time_left = m_asteroid_spawn_time_left;
    time_left -= dt;
    if (time_left <= 0.0f) {
        m_asteroids.emplace_back(Asteroid());
        m_asteroid_spawn_time_left = k_asteroid_spawn_timer;
    }
    else {
        m_asteroid_spawn_time_left = time_left;
    }
}

void
Fasteroids::DespawnDistantLazers()
{
    // Todo: don't hardcode this
    float dist = 5.0f;
    AABB aabb = {
        0.0f - dist,
        0.0f - dist,
        4.0f + dist,
        3.0f + dist
    };

    auto it = m_lazers.begin();
    while (it < m_lazers.end()) {
        while (it < m_lazers.end()) {
            V2F32 pos = {it->circle.x, it->circle.y};
            if (!v2f32_inside_aabb(pos, aabb)) {
                std::iter_swap(it, m_lazers.end()-1);
                m_lazers.pop_back();
            }
            else {
                break;
            }
        }
        it++;
    }

}

void
Fasteroids::DespawnDistantAsteroids()
{
    // Todo: don't hardcode this
    float dist = 5.0f;
    AABB aabb = {
        0.0f - dist,
        0.0f - dist,
        4.0f + dist,
        3.0f + dist
    };

    auto it = m_asteroids.begin();
    while (it < m_asteroids.end()) {
        while (it < m_asteroids.end()) {
            V2F32 pos = it->GetPos();
            if (!v2f32_inside_aabb(pos, aabb)) {
                std::iter_swap(it, m_asteroids.end()-1);
                m_asteroids.pop_back();
            }
            else {
                break;
            }
        }
        it++;
    }
}

void
Fasteroids::Draw()
{
    Color bg_color = {0.01f, 0.01f, 0.01f};
    g_renderer.SetClearColor(bg_color);


    m_ship.Draw();

    for (Asteroid& asteroid : m_asteroids) {
        asteroid.Draw();
    }

    for (Lazer& lazer : m_lazers) {
        lazer.Draw();
    }
}

