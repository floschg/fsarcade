#include "common/MemoryManager.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/RSoftwareBackend.hpp"

#include <glad/gl.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <immintrin.h>


RSoftwareBackend::RSoftwareBackend(Renderer& renderer)
    : m_renderer {renderer}
{
    m_canvas.rshift = 0;
    m_canvas.gshift = 8;
    m_canvas.bshift = 16;
    m_canvas.ashift = 24;
    m_canvas.w = 0;
    m_canvas.h = 0;
    m_canvas.pixels = nullptr;


    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);

    glGenTextures(1, &m_gltexture_id);
    glBindTexture(GL_TEXTURE_2D, m_gltexture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void
RSoftwareBackend::Draw()
{
    Resize(m_renderer.m_screen_w, m_renderer.m_screen_h);
    DrawClear();

    SortRenderEntities();

    for (auto& sort_entry : m_renderer.m_sort_entries) {
        REntity& entity = m_renderer.m_render_entities[sort_entry.entity_index];
        switch (entity.type) {
        case REntityType_Rectangle: {
            DrawRectangle(entity.rect);
        } break;

        case REntityType_AlphaBitmap: {
            DrawAlphaBitmap(entity.bitmap);
        } break;

        case REntityType_Text: {
            DrawFrameString32(entity.string32);
        }; break;

        case REntityType_Circle: {
            DrawCircle(entity.circle);
        }; break;

        default:;
        }
    }


    glBindTexture(GL_TEXTURE_2D, m_gltexture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_canvas.w, m_canvas.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_canvas.pixels);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();
}

void
RSoftwareBackend::Resize(int32_t w, int32_t h)
{
    if ((m_canvas.w == w && m_canvas.h == h)) {
        return;
    }

    size_t alignment = 32;
    size_t new_size = static_cast<size_t>(w) * static_cast<size_t>(h) * sizeof(m_canvas.pixels[0]);
    if (new_size == 0) {
        _mm_free(m_canvas.pixels);
        m_canvas.pixels = nullptr;
        m_canvas.w = 0;
        m_canvas.h = 0;
        return;
    }

    uint32_t* new_pixels = (uint32_t*)_mm_malloc(new_size, alignment);
    if (!new_pixels) {
        printf("_mm_malloc failed for resizing canvas\n");
        return;
    }

    _mm_free(m_canvas.pixels);
    m_canvas.w = w;
    m_canvas.h = h;
    m_canvas.pixels = new_pixels;

    glViewport(0, 0, w, h);
}

void
RSoftwareBackend::SortRenderEntities()
{
    auto& z_buff = m_renderer.m_sort_entries;
    std::sort(z_buff.begin(), z_buff.end(),
              [](const RSortEntry& e1, const RSortEntry& e2) {
                  return e1.z < e2.z;
              });
}

void
RSoftwareBackend::DrawClear()
{
    Color color = m_renderer.m_clear_color;

    uint32_t r = static_cast<uint32_t>(color.r * 255.0f) << m_canvas.rshift;
    uint32_t g = static_cast<uint32_t>(color.g * 255.0f) << m_canvas.gshift;
    uint32_t b = static_cast<uint32_t>(color.b * 255.0f) << m_canvas.bshift;
    uint32_t val = r | g | b;

    size_t pixel_count = size_t(m_canvas.w) * size_t(m_canvas.h);
    size_t chunk_count = pixel_count / 8;
    size_t chunk_rest = pixel_count % 8;
    uint32_t* pixels = m_canvas.pixels;

    __m256i vec_val = _mm256_set1_epi32((int32_t)val);
    for (size_t i = 0; i < chunk_count; i++) {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(pixels), vec_val);
        pixels += 8;
    }

    for (size_t i = 0; i < chunk_rest; i++) {
        *pixels++ = val;
    }
}

void
RSoftwareBackend::DrawRectangle(REntity_Rectangle& entity)
{
    int32_t xmin = m_renderer.WorldXToScreenX(entity.rect.x0);
    int32_t ymin = m_renderer.WorldYToScreenY(entity.rect.y0);
    int32_t xmax = m_renderer.WorldXToScreenX(entity.rect.x1);
    int32_t ymax = m_renderer.WorldYToScreenY(entity.rect.y1);

    if (xmin < 0) {
        xmin = 0;
    }
    if (ymin < 0) {
        ymin = 0;
    }
    if (xmax >= m_canvas.w) {
        xmax = m_canvas.w - 1;
    }
    if (ymax >= m_canvas.h) {
        ymax = m_canvas.h - 1;
    }

    uint32_t r = static_cast<uint32_t>(entity.color.r * 255.0f) << m_canvas.rshift;
    uint32_t g = static_cast<uint32_t>(entity.color.g * 255.0f) << m_canvas.gshift;
    uint32_t b = static_cast<uint32_t>(entity.color.b * 255.0f) << m_canvas.bshift;
    uint32_t val = r | g | b;

    for (int32_t y = ymin; y <= ymax; ++y) {
        uint32_t *pixel = m_canvas.pixels + y * m_canvas.w + xmin;
        for (int32_t x = xmin; x <= xmax; ++x) {
            *pixel++ = val;
        }
    }
}

void
RSoftwareBackend::DrawCircle(REntity_Circle& entity)
{
    // center + radius
    int32_t cx = m_renderer.WorldXToScreenX(entity.circle.x);
    int32_t cy = m_renderer.WorldYToScreenY(entity.circle.y);
    int32_t r = m_renderer.WorldXToScreenX(entity.circle.x + entity.circle.r) - m_renderer.WorldXToScreenX(entity.circle.x);

    // digital differential analyser variables
    int32_t r2 = r + r;
    int32_t D = r2 - 1;
    int32_t D_dy  = -2;
    int32_t D_ddy = -4;
    int32_t D_dx  = r2 + r2 -4;
    int32_t D_ddx = -4;

    int32_t y = 0;
    int32_t x = r;
    while (y < x) {
        DrawHorizontalLine_Screen(cx, cx + x, cy + y, entity.color);
        DrawHorizontalLine_Screen(cx, cx - x, cy + y, entity.color);
        DrawHorizontalLine_Screen(cx, cx + x, cy - y, entity.color);
        DrawHorizontalLine_Screen(cx, cx - x, cy - y, entity.color);

        DrawHorizontalLine_Screen(cx, cx + y, cy + x, entity.color);
        DrawHorizontalLine_Screen(cx, cx - y, cy + x, entity.color);
        DrawHorizontalLine_Screen(cx, cx + y, cy - x, entity.color);
        DrawHorizontalLine_Screen(cx, cx - y, cy - x, entity.color);

        D += D_dy;
        D_dy += D_ddy;
        y++;

        // if moving left:     all bits are 1
        // if not moving left: all bits are 0
        // all bits 1 also represents -1, so we can add it to x
        int32_t mask = D >> 31;
        D += D_dx & mask;
        D_dx += D_ddx & mask;
        x += mask;
    }
}

void
RSoftwareBackend::DrawAlphaBitmap(REntity_AlphaBitmap& entity)
{
    int32_t x0 = m_renderer.WorldXToScreenX(entity.pos.x);
    int32_t y0 = m_renderer.WorldYToScreenY(entity.pos.y);
    int32_t x1 = x0 + entity.bitmap.w - 1;
    int32_t y1 = y0 + entity.bitmap.h - 1;

    int32_t cut_left = 0;
    int32_t cut_bot = 0;

    if (x0 < 0) {
        cut_left = x0;
        x0 = 0;
    }
    if (y0 < 0) {
        cut_bot = y0;
        y0 = 0;
    }
    if (x1 >= m_canvas.w) {
        x1 = m_canvas.w - 1;
    }
    if (y1 >= m_canvas.h) {
        y1 = m_canvas.h - 1;
    }


    uint8_t* alpha_row = (uint8_t*)entity.bitmap.pixels.get() + (-cut_bot * entity.bitmap.w) + (-cut_left);
    uint32_t* rgba_row = m_canvas.pixels + y0 * m_canvas.w + x0;
    for (int32_t y = y0; y <= y1; y++) {
        uint8_t* alpha = alpha_row;
        uint32_t* rgba = rgba_row;
        for (int32_t x = x0; x <= x1; x++) {
            if (*alpha == 0) {
                alpha++;
                rgba++;
                continue;
            }

            float alphaf = *alpha / 255.0f;

            float r1 = entity.color.r * alphaf;
            float g1 = entity.color.g * alphaf;
            float b1 = entity.color.b * alphaf;

            uint32_t r2 = static_cast<uint32_t>(r1 * 255.0f) << m_canvas.rshift;
            uint32_t g2 = static_cast<uint32_t>(g1 * 255.0f) << m_canvas.gshift;
            uint32_t b2 = static_cast<uint32_t>(b1 * 255.0f) << m_canvas.bshift;
            uint32_t rgba_result = r2 | g2 | b2;
            *rgba = rgba_result;

            alpha++;
            rgba++;
        }

        alpha_row += entity.bitmap.w;
        rgba_row += m_canvas.w;
    }
}

void
RSoftwareBackend::DrawFrameString32(REntity_String32& entity)
{
    int32_t xscreen = m_renderer.WorldXToScreenX(entity.pos.x);
    int32_t yscreen = m_renderer.WorldYToScreenY(entity.pos.y);
    std::u32string& str = MemoryManager::GetString32(entity.id);
    for (char32_t ch : str) {
        Glyph& glyph = entity.font.GetGlyph(ch);

        int32_t x = xscreen + glyph.xoff;
        int32_t y = yscreen + glyph.yoff;
        DrawTextGlyph(glyph, entity.color, x, y);

        xscreen += glyph.xadvance;
    }
}

void
RSoftwareBackend::DrawTextGlyph(Glyph& glyph, Color color, int32_t xscreen, int32_t yscreen)
{
    int32_t x0 = xscreen;
    int32_t y0 = yscreen;
    int32_t x1 = x0 + glyph.bitmap.w - 1;
    int32_t y1 = y0 + glyph.bitmap.h - 1;

    int32_t cut_left = 0;
    int32_t cut_bot = 0;

    if (x0 < 0) {
        cut_left = x0;
        x0 = 0;
    }
    if (y0 < 0) {
        cut_bot = y0;
        y0 = 0;
    }
    if (x1 >= m_canvas.w) {
        x1 = m_canvas.w - 1;
    }
    if (y1 >= m_canvas.h) {
        y1 = m_canvas.h - 1;
    }


    uint8_t* alpha_row = (uint8_t*)glyph.bitmap.pixels.get() + (-cut_bot * glyph.bitmap.w) + (-cut_left);
    uint32_t* rgba_row = m_canvas.pixels + y0 * m_canvas.w + x0;

    for (int32_t y = y0; y <= y1; y++) {
        uint8_t* alpha = alpha_row;
        uint32_t* rgba = rgba_row;
        for (int32_t x = x0; x <= x1; x++) {
            if (*alpha == 0) {
                alpha++;
                rgba++;
                continue;
            }

            float alphaf = *alpha / 255.0f;

            float r1 = color.r * alphaf;
            float g1 = color.g * alphaf;
            float b1 = color.b * alphaf;

            uint32_t r2 = static_cast<uint32_t>(r1 * 255.0f) << m_canvas.rshift;
            uint32_t g2 = static_cast<uint32_t>(g1 * 255.0f) << m_canvas.gshift;
            uint32_t b2 = static_cast<uint32_t>(b1 * 255.0f) << m_canvas.bshift;
            uint32_t rgba_result = r2 | g2 | b2;
            *rgba = rgba_result;

            alpha++;
            rgba++;
        }

        alpha_row += glyph.bitmap.w;
        rgba_row += m_canvas.w;
    }
}

void
RSoftwareBackend::DrawHorizontalLine_Screen(int32_t x0, int32_t x1, int32_t y, Color color)
{
    if (y < 0 || y >= m_canvas.h) {
        return;
    }

    uint32_t r = static_cast<uint32_t>(color.r * 255.0f) << m_canvas.rshift;
    uint32_t g = static_cast<uint32_t>(color.g * 255.0f) << m_canvas.gshift;
    uint32_t b = static_cast<uint32_t>(color.b * 255.0f) << m_canvas.bshift;
    uint32_t pixel_val = r | g | b;

    int32_t xmin = std::max(std::min(x0, x1), 0);
    int32_t xmax = std::min(std::max(x0, x1), m_canvas.w-1);
    for (int32_t x = xmin; x < xmax; x++) {
        m_canvas.pixels[y * m_canvas.w + x] = pixel_val;
    }
}

