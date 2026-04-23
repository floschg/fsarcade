#include "common/MemoryManager.hpp"
#include "common/Jobsys.hpp"
#include "common/math.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/RSoftwareBackend.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_pixels.h>

#include <algorithm>
#include <immintrin.h>
#include <cstdlib>
#include <cstdio>
#include <cmath>


RSoftwareBackend::RSoftwareBackend(Renderer& renderer)
    : m_renderer {renderer}
{
    assert(renderer.m_screen_w != 0);
    assert(renderer.m_screen_h != 0);
    Resize(renderer.m_screen_w, renderer.m_screen_h);

    m_texture = SDL_CreateTexture(m_renderer.m_sdl_renderer,
                                  m_surface->format,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  m_surface->w,
                                  m_surface->h);
    if (!m_texture) {
        printf("SDL_CreateTexture failed\n");
        exit(0);
    }
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
        case REntityType::AABB: {
            DrawAABB(entity.aabb);
        } break;

        case REntityType::AlphaBitmap: {
            DrawAlphaBitmap(entity.bitmap);
        } break;

        case REntityType::Text: {
            DrawFrameString32(entity.string32);
        }; break;

        case REntityType::Circle: {
            DrawCircle(entity.circle);
        }; break;

        case REntityType::Mesh: {
            DrawMesh(entity.mesh);
        }; break;

        default:;
        }
    }

    if (!SDL_UpdateTexture(m_texture, 0, m_surface->pixels, m_surface->pitch)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_UpdateTexture failed: %s\n", SDL_GetError());
    }
    if (!SDL_RenderTextureRotated(m_renderer.m_sdl_renderer, m_texture, 0, 0, 0.0f, 0, SDL_FLIP_VERTICAL)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_RenderTexture failed: %s\n", SDL_GetError());
    }
}

void
RSoftwareBackend::Resize(int32_t w, int32_t h)
{
    int alignment = 32;
    int bytes_per_pixel = 4;

    int pitch_unaligned = w * bytes_per_pixel;
    int pitch = (pitch_unaligned + alignment-1) & ~(alignment-1);
    int size = h * pitch;

    void* pixels = _mm_malloc((size_t)size, (size_t)alignment);
    SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_ARGB8888, pixels, pitch);
    if (!surface) {
        printf("SDL_CreateSurfaceFrom failed on Resize\n");
        _mm_free(pixels);
        return;
    }
    if (m_surface) {
        void* old_pixels = m_surface->pixels;
        SDL_DestroySurface(m_surface);
        _mm_free(old_pixels);
        m_surface = 0;
    }

    m_surface = surface;
    assert((uint64_t)m_surface->pixels % (uint64_t)alignment == 0);
    assert(m_surface->pitch % alignment == 0);
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

struct DrawClearMultithreadedData {
    uint8_t* pixel_row;
    int pitch;
    int row_count;
    int row_chunk_count;
    int row_chunk_rest;
    uint32_t color_val;
    __m256i color_vec;
};

static void
DrawClearMultithreaded(void* _data)
{
    DrawClearMultithreadedData* data = (DrawClearMultithreadedData*)_data;
    uint8_t* pixel_row = (uint8_t*)data->pixel_row;
    int pitch = data->pitch;
    int row_count = data->row_count;
    int row_chunk_count = data->row_chunk_count;
    int row_chunk_rest = data->row_chunk_rest;
    uint32_t color_val = data->color_val;
    __m256i color_vec = data->color_vec;

    for (int y = 0; y < row_count; y++) {
        uint32_t *pixels = (uint32_t*)pixel_row;

        for (int i = 0; i < row_chunk_count; i++) {
            _mm256_store_si256((__m256i*)pixels, color_vec);
            pixels += 8;
        }

        for (int i = 0; i < row_chunk_rest; i++) {
            *pixels++ = color_val;
        }

        pixel_row += pitch;
    }
}

void
RSoftwareBackend::DrawClear()
{
    Color color = m_renderer.m_clear_color;

    uint8_t* pixel_row = (uint8_t*)m_surface->pixels;
    int pitch = m_surface->pitch;
    int row_count = m_surface->h;
    int row_chunk_count = m_surface->w / 8;
    int row_chunk_rest = m_surface->w % 8;
    uint32_t color_val = MapRGB(color.r, color.g, color.b);
    __m256i color_vec = _mm256_set1_epi32((int32_t)color_val);

    int rows_per_job = row_count / 4;
    int rows_per_job_rest = row_count % 4;
    int rows_last_job = rows_per_job + rows_per_job_rest;
    DrawClearMultithreadedData datas[] = {
        {
            pixel_row + (0 * pitch * rows_per_job), pitch, rows_per_job,
            row_chunk_count, row_chunk_rest, color_val, color_vec
        },
        {
            pixel_row + (1 * pitch * rows_per_job), pitch, rows_per_job,
            row_chunk_count, row_chunk_rest, color_val, color_vec
        },
        {
            pixel_row + (2 * pitch * rows_per_job), pitch, rows_per_job,
            row_chunk_count, row_chunk_rest, color_val, color_vec
        },
        {
            pixel_row + (3 * pitch * rows_per_job), pitch, rows_last_job,
            row_chunk_count, row_chunk_rest, color_val, color_vec
        },
    };
    Job jobs[] = {
        {DrawClearMultithreaded, datas+3},
        {DrawClearMultithreaded, datas+2},
        {DrawClearMultithreaded, datas+1},
        {DrawClearMultithreaded, datas+0},
    };
    g_jobsys->StartJob(jobs);
    g_jobsys->StartJob(jobs+1);
    g_jobsys->StartJob(jobs+2);
    g_jobsys->StartJob(jobs+3);

    g_jobsys->FinishJob(jobs);
    g_jobsys->FinishJob(jobs+1);
    g_jobsys->FinishJob(jobs+2);
    g_jobsys->FinishJob(jobs+3);
}

void
RSoftwareBackend::DrawAABB(REntity_AABB& entity)
{
    int32_t xmin = m_renderer.WorldXToScreenX(entity.aabb.x0);
    int32_t ymin = m_renderer.WorldYToScreenY(entity.aabb.y0);
    int32_t xmax = m_renderer.WorldXToScreenX(entity.aabb.x1);
    int32_t ymax = m_renderer.WorldYToScreenY(entity.aabb.y1);

    if (xmin < 0) {
        xmin = 0;
    }
    if (ymin < 0) {
        ymin = 0;
    }
    if (xmax >= m_surface->w) {
        xmax = m_surface->w - 1;
    }
    if (ymax >= m_surface->h) {
        ymax = m_surface->h - 1;
    }

    uint32_t val = MapRGB(entity.color.r, entity.color.g, entity.color.b);

    uint8_t* pixel_row = (uint8_t*)m_surface->pixels + ymin*m_surface->pitch;
    for (int32_t y = ymin; y <= ymax; ++y) {
        uint32_t* pixel = (uint32_t*)(pixel_row + xmin*4);
        for (int32_t x = xmin; x <= xmax; ++x) {
            *pixel++ = val;
        }
        pixel_row += m_surface->pitch;
    }
}

void
RSoftwareBackend::DrawCircle(REntity_Circle& entity)
{
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
    while (y <= x) {
        // top,  0-45 degrees
        DrawHorizontalLine_Screen(cx, cx + x, cy + y, entity.color);
        DrawHorizontalLine_Screen(cx, cx - x, cy + y, entity.color);

        // top, 45-90 degrees
        DrawHorizontalLine_Screen(cx, cx + y, cy + x, entity.color);
        DrawHorizontalLine_Screen(cx, cx - y, cy + x, entity.color);

        // bot,  0-45 degrees
        DrawHorizontalLine_Screen(cx, cx + x, cy - y, entity.color);
        DrawHorizontalLine_Screen(cx, cx - x, cy - y, entity.color);

        // bot, 45-90 degrees
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
    if (x1 >= m_surface->w) {
        x1 = m_surface->w - 1;
    }
    if (y1 >= m_surface->h) {
        y1 = m_surface->h - 1;
    }


    uint8_t* alpha_row = (uint8_t*)entity.bitmap.pixels.get() + (-cut_bot * entity.bitmap.w) + (-cut_left);
    uint8_t* pixel_row = (uint8_t*)m_surface->pixels + y0 * m_surface->pitch + x0*4;
    for (int32_t y = y0; y <= y1; y++) {
        uint8_t* alpha = alpha_row;
        uint32_t* pixel = (uint32_t*)pixel_row;

        for (int32_t x = x0; x <= x1; x++) {
            if (*alpha == 0) {
                alpha++;
                pixel++;
                continue;
            }

            float alphaf = *alpha / 255.0f;
            float r1 = entity.color.r * alphaf;
            float g1 = entity.color.g * alphaf;
            float b1 = entity.color.b * alphaf;

            uint32_t val = MapRGB(r1, g1, b1);
            *pixel = val;

            alpha++;
            pixel++;
        }

        alpha_row += entity.bitmap.w;
        pixel_row += m_surface->pitch;
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
    // Todo: Why not just call DrawAlphaBitmap????
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
    if (x1 >= m_surface->w) {
        x1 = m_surface->w - 1;
    }
    if (y1 >= m_surface->h) {
        y1 = m_surface->h - 1;
    }


    uint8_t* alpha_row = (uint8_t*)glyph.bitmap.pixels.get() + (-cut_bot * glyph.bitmap.w) + (-cut_left);
    uint8_t* rgba_row = (uint8_t*)m_surface->pixels + y0 * m_surface->pitch + x0*4;

    for (int32_t y = y0; y <= y1; y++) {
        uint8_t* alpha = alpha_row;
        uint32_t* rgba = (uint32_t*)rgba_row;
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

            uint32_t val = MapRGB(r1, g1, b1);
            *rgba = val;

            alpha++;
            rgba++;
        }

        alpha_row += glyph.bitmap.w;
        rgba_row += m_surface->pitch;
    }
}

void
RSoftwareBackend::DrawTriangle(float* vertices, Color color)
{
    V2F32 v0 = {vertices[0], vertices[1]};
    V2F32 v1 = {vertices[2], vertices[3]};
    V2F32 v2 = {vertices[4], vertices[5]};
    if (v1.y < v0.y) {
        std::swap(v1, v0);
    }
    if (v2.y < v0.y) {
        std::swap(v2, v0);
    }
    if (v2.y < v1.y) {
        std::swap(v2, v1);
    }

    V2I32 pos0 = {m_renderer.WorldXToScreenX(v0.x), m_renderer.WorldYToScreenY(v0.y)};
    V2I32 pos1 = {m_renderer.WorldXToScreenX(v1.x), m_renderer.WorldYToScreenY(v1.y)};
    V2I32 pos2 = {m_renderer.WorldXToScreenX(v2.x), m_renderer.WorldYToScreenY(v2.y)};

    float dist01 = float(pos1.y - pos0.y);
    float dist12 = float(pos2.y - pos1.y);
    float dist02 = float(pos2.y - pos0.y);

    // @Speed: precompute delta-x's
    // @Incomplete: draw topmost line (y = pos2.y); Take care of not dividing by 0.

    int32_t y = pos0.y;
    while (y < pos1.y) {
        float y01_progress = 1.0f - (float)(pos1.y - y) / dist01;
        float y02_progress = 1.0f - (float)(pos2.y - y) / dist02;
        int32_t x01 = pos0.x + (int)(y01_progress * (float(pos1.x - pos0.x)));
        int32_t x02 = pos0.x + (int)(y02_progress * (float(pos2.x - pos0.x)));
        DrawHorizontalLine_Screen(x01, x02, y, color);
        y++;
    }
    while (y < pos2.y) {
        float y12_progress = 1.0f - (float)(pos2.y - y) / dist12;
        float y02_progress = 1.0f - (float)(pos2.y - y) / dist02;
        int32_t x12 = pos1.x + (int)(y12_progress * (float(pos2.x - pos1.x)));
        int32_t x02 = pos0.x + (int)(y02_progress * (float(pos2.x - pos0.x)));
        DrawHorizontalLine_Screen(x12, x02, y, color);
        y++;
    }
}

void
RSoftwareBackend::DrawMesh(REntity_Mesh& entity)
{
    float a = entity.angle;

    // Todo: use quaternions
    Mat4x4 rotation_z = {
        {std::cos(a), -std::sin(a), 0.0f, 0.0f},
        {std::sin(a),  std::cos(a), 0.0f, 0.0f},
        {0.0f,        0.0f,         1.0f, 0.0f},
        {0.0f,        0.0f,         0.0f, 1.0f},
    };
    Mat4x4 translation = {
        {1.0f, 0.0f, 0.0f, entity.pos.x},
        {0.0f, 1.0f, 0.0f, entity.pos.y},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    Mat4x4 transform;
    mat4x4_dot_mat4x4(translation, rotation_z, transform);

    V2F32* positions = (V2F32*)entity.mesh.m_vertices.data();
    uint32_t* indices = entity.mesh.m_indices.data();
    for (size_t i = 0; i < entity.mesh.m_indices.size(); i+=3) {

        V2F32 result_positions[3];
        result_positions[0] = mat4x4_dot_v2f32(transform, &positions[indices[i+0]]);
        result_positions[1] = mat4x4_dot_v2f32(transform, &positions[indices[i+1]]);
        result_positions[2] = mat4x4_dot_v2f32(transform, &positions[indices[i+2]]);
        DrawTriangle((float*)result_positions, entity.color);
    }
}

void
RSoftwareBackend::DrawHorizontalLine_Screen(int32_t x0, int32_t x1, int32_t y, Color color)
{
    if (y < 0 || y >= m_surface->h) {
        return;
    }

    uint32_t val = MapRGB(color.r, color.g, color.b);

    uint32_t* pixels = (uint32_t*)m_surface->pixels;
    int32_t xmin = std::max(std::min(x0, x1), 0);
    int32_t xmax = std::min(std::max(x0, x1), m_surface->w-1);
    for (int32_t x = xmin; x < xmax; x++) {
        pixels[y * m_surface->w + x] = val;
    }
}

uint32_t
RSoftwareBackend::MapRGB(float r, float g, float b)
{
    uint32_t val = SDL_MapRGBA(SDL_GetPixelFormatDetails(m_surface->format),
                               0,
                               (Uint8)(r * 255.0f),
                               (Uint8)(g * 255.0f),
                               (Uint8)(b * 255.0f),
                               255);
    return val;
}

