#include "games/fasteroids/Fasteroids.hpp"
#include "common/math.hpp"
#include "renderer/Renderer.hpp"


static constexpr float k_pi = static_cast<float>(std::numbers::pi);


std::unique_ptr<Game>
Game::CreateFasteroids()
{
    return std::make_unique<Fasteroids>();
}

void
Fasteroids::Start()
{
    g_renderer.SetCameraSize(4.0f, 3.0f);

    m_asteroids.clear();
    m_asteroids.push_back(Asteroid());

    m_game_status = game_resume;
}

void
Fasteroids::ProcessEvent(SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        SDL_Keycode keycode = event.key.key;
        if (keycode == SDLK_LEFT) {
            m_ship.RotateCounterClockwise(k_pi/32);
        }
        else if (keycode == SDLK_RIGHT) {
            m_ship.RotateClockwise(k_pi/32);
        }
        else if (keycode == SDLK_UP) {
            m_ship.BoostForward();
        }
        else if (keycode == SDLK_SPACE) {
            m_lazers.emplace_back(m_ship.ShootLazer());
        }
    }
    }
}

void
Fasteroids::Update(float dt)
{
    m_ship.Update(dt);

    for (Lazer& lazer : m_lazers) {
        lazer.Update(dt);
    }

    AABB aabb_world = {
        0.0f, 0.0f,
        4.0f, 3.0f
    };

    auto it = m_lazers.begin();
    while (it < m_lazers.end()) {
        while (it < m_lazers.end()) {
            if (!Intersect_AABB_Circle(aabb_world, it->circle)) {
                std::iter_swap(it, m_lazers.end()-1);
                m_lazers.pop_back();
            }
            else {
                break;
            }
        }
        it++;
    }

    for (Asteroid& asteroid : m_asteroids) {
        asteroid.Update(dt);
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

