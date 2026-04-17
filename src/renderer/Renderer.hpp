#pragma once

#include "common/math.hpp"
#include "common/Font.hpp"
#include "common/MemoryManager.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <vector>
#include <memory>


class RSoftwareBackend;
class Renderer;

extern Renderer g_renderer;


enum class REntityType : int32_t {
    AABB,
    AlphaBitmap,
    Circle,
    Text,
};


struct REntity_AlphaBitmap {
    REntityType type;
    AlphaBitmap& bitmap;
    V2F32 pos;
    Color color;
};

struct REntity_AABB {
    REntityType type;
    AABB aabb;
    Color color;
};

struct RENtity_Rect {
    REntityType type;
    AABB aabb;
    Color color;
    float angle;
};

struct REntity_Circle {
    REntityType type;
    Circle circle;
    Color color;
};

struct REntity_String32 {
    REntityType type;
    String32Id id;
    Font& font;
    V2F32 pos;
    Color color;
};


union REntity {
    REntityType type;
    REntity_AlphaBitmap bitmap;
    REntity_AABB aabb;
    REntity_Circle circle;
    REntity_String32 string32;
};

struct RSortEntry {
    uint32_t z;
    uint32_t entity_index;
};


class Renderer {
public:
    void Init(SDL_Window* window, SDL_Renderer* sdl_renderer);


    /* core functions */

    void Draw();
    void Reset();

    void SetClearColor(Color color);
    void SetScreenSize(int32_t w, int32_t h);
    void SetCameraSize(float w, float h);

    void PushAlphaBitmap(AlphaBitmap& bitmap, V2F32 pos, Color color, uint32_t z);
    void PushAABB(AABB aabb, Color color, uint32_t z);
    void PushCircle(Circle circle, Color color, uint32_t z);
    void PushString32(String32Id id, Font& font, V2F32 pos, Color color, uint32_t z);


    /* helper functions */

    int32_t WorldXToScreenX(float x);
    int32_t WorldYToScreenY(float y);
    int32_t WorldWidthToScreenWidth(float w);
    int32_t WorldHeightToScreenHeight(float h);

    V2F32 ScreenPosToViewPos(V2F32 screen_pos);



public:
    SDL_Window* m_window;
    SDL_Renderer* m_sdl_renderer;

    int32_t m_screen_w;
    int32_t m_screen_h;

    float m_camera_w;
    float m_camera_h;

    Color m_clear_color {};
    std::vector<REntity> m_render_entities;
    std::vector<RSortEntry> m_sort_entries;

    std::unique_ptr<RSoftwareBackend> m_backend;
    friend class RSoftwareBackend;
};

