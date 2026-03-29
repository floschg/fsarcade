#include "renderer/Renderer.hpp"
#include "renderer/RSoftwareBackend.hpp"

#include <imgui.h>

#include <memory>


Renderer g_renderer;


void
Renderer::Init(SDL_Window* window)
{
    m_window = window;
    m_render_entities.reserve(1024);
    m_sort_entries.reserve(1024);
    m_backend = std::make_unique<RSoftwareBackend>(*this);
}

void
Renderer::Reset()
{
    m_render_entities.clear();
    m_sort_entries.clear();
    SetCameraSize(0.0f, 0.0f);
}

void
Renderer::Draw()
{
    m_backend->Draw();
}

void
Renderer::SetScreenSize(int32_t w, int32_t h)
{
    m_screen_w = w;
    m_screen_h = h;
}

void
Renderer::SetCameraSize(float w, float h)
{
    m_camera_w = w;
    m_camera_h = h;
}

int32_t
Renderer::WorldXToScreenX(float world_x)
{
    float screen_x = (world_x / m_camera_w) * (float)m_screen_w;
    return (int32_t)screen_x;
}

int32_t
Renderer::WorldYToScreenY(float world_y)
{
    float screen_y = (world_y / m_camera_h) * (float)m_screen_h;
    return (int32_t)screen_y;
}


V2F32
Renderer::ScreenPosToViewPos(V2F32 screen_pos)
{
    V2F32 view_pos = {};
    view_pos.x = (screen_pos.x / (float)m_screen_w) * m_camera_w;
    view_pos.y = (screen_pos.y / (float)m_screen_h) * m_camera_h;
    return view_pos;
}

void
Renderer::SetClearColor(Color color)
{
    m_clear_color = color;
}

void
Renderer::PushAlphaBitmap(AlphaBitmap& bitmap, V2F32 pos, Color color, uint32_t z)
{
    m_render_entities.emplace_back(REntity{.bitmap{
            REntityType_AlphaBitmap,
            bitmap,
            pos,
            color
    }});
    m_sort_entries.emplace_back(z, m_render_entities.size()-1);
}

void
Renderer::PushRectangle(Rectangle rect, Color color, uint32_t z)
{
    m_render_entities.emplace_back(REntity{.rect{
            REntityType_Rectangle,
            rect,
            color
    }});
    m_sort_entries.emplace_back(z, m_render_entities.size()-1);
}

void
Renderer::PushCircle(Circle circle, Color color, uint32_t z)
{
    m_render_entities.emplace_back(REntity{.circle{
            REntityType_Circle,
            circle,
            color
    }});
    m_sort_entries.emplace_back(z, m_render_entities.size()-1);
}

void
Renderer::PushString32(String32Id id, Font& font, V2F32 pos, Color color, uint32_t z)
{
    m_render_entities.emplace_back(REntity{.string32{
            REntityType_Text,
            id,
            font,
            pos,
            color
    }});
    m_sort_entries.emplace_back(z, m_render_entities.size()-1);
}


