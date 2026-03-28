#pragma once

#include "renderer/Renderer.hpp"

#include <SDL3/SDL.h>


class RSoftwareBackend {
public:
    struct Canvas {
        uint32_t rshift;
        uint32_t gshift;
        uint32_t bshift;
        uint32_t ashift;
        int32_t w;
        int32_t h;
        uint32_t* pixels;
    };


public:
    RSoftwareBackend(Renderer& renderer);

    void Draw();


private:
    void Resize(int32_t w, int32_t h);
    void SortRenderEntities();

    void DrawClear();
    void DrawRectangle(REntity_Rectangle& entity);
    void DrawCircle(REntity_Circle& circle);
    void DrawAlphaBitmap(REntity_AlphaBitmap& entity);
    void DrawFrameString32(REntity_String32& entity);
    void DrawTextGlyph(Glyph& glyph, Color color, int32_t xscreen, int32_t yscreen);

    void DrawHorizontalLine_Screen(int32_t x0, int32_t x1, int32_t y, Color color);

private:
    Renderer& m_renderer;

    uint32_t m_gltexture_id{};
    Canvas m_canvas;
};

