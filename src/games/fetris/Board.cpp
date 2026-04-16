#include "games/fetris/Fetris.hpp"
#include "games/fetris/Board.hpp"
#include "games/fetris/Fetromino.hpp"
#include "renderer/Renderer.hpp"

void
Board::Reset()
{
    for (int y = 0; y < 2; y++) {
        m_bitmap[y] = 0xffff; // 1111111111111111
    }
    for (int y = 2; y < 24; y++) {
        m_bitmap[y] = 0xe007; // 1110000000000111
    }

    for (int y = 0; y < 22; y++) {
        for (int x = 0; x < 10; x++) {
            m_idmap[y][x] = Fetromino::id_none;
        }
    }
}

int32_t
Board::PlaceFetromino(Fetromino &fetromino)
{
    BoardPos pos = fetromino.GetPos();
    Fetromino::Id id = fetromino.GetId();
    uint16_t fetromino_bitmap[4];
    fetromino.GetBitmap(fetromino_bitmap);


    // place in Board's Bitmap
    m_bitmap[pos.y+0] |= fetromino_bitmap[0];
    m_bitmap[pos.y+1] |= fetromino_bitmap[1];
    m_bitmap[pos.y+2] |= fetromino_bitmap[2];
    m_bitmap[pos.y+3] |= fetromino_bitmap[3];


    // place in Board's Idmap
    for (int32_t y = 0; y < 4; y++) {
        for (int32_t x = 0; x < 4; x++) {
            int32_t bitmap_x = 0x8000 >> (pos.x + x);
            if (fetromino_bitmap[y] & bitmap_x) {
                int32_t idmap_x = pos.x + x - 3;
                int32_t idmap_y = pos.y + y - 2;
                m_idmap[idmap_y][idmap_x] = id;
            }
        }
    }

    int32_t rows_cleared = ClearRows(pos.y);
    return rows_cleared;
}

int32_t
Board::ClearRows(int32_t y0)
{
    int32_t rows_cleared = 0;
    int32_t y1 = y0 + 3;

    // ignore for y = {0,1}. Those bitmap rows are all 1's for collision testing
    if (y0 < 2) {
        y0 += 2 - y0;
    }

    for (int32_t y = y0; y <= y1; y++) {
        if (m_bitmap[y] == 0xffff) {
            rows_cleared++;
        }
        else {
            m_bitmap[y-rows_cleared] = m_bitmap[y];
            std::copy(m_idmap[y-2], m_idmap[y-2] + 10, m_idmap[y-2-rows_cleared]);
        }
    }
    for (int32_t y = y1+1; y < 24; y++) {
        m_bitmap[y-rows_cleared] = m_bitmap[y];
        std::copy(m_idmap[y-2], m_idmap[y-2] + 10, m_idmap[y-2-rows_cleared]);
    }
    for (int32_t y = 24-rows_cleared; y < 24; y++) {
        m_bitmap[y] = 0xe007;
        std::fill(m_idmap[y-2], m_idmap[y-2] + 10, Fetromino::id_none);
    }


    return rows_cleared;
}

void
Board::Draw(int32_t level)
{
    float world_width = 4.0f;
    float world_height = 3.0f;
    float fetromino_size_with_border = world_height / 20.0f;
    float fetromino_size = 0.8f * fetromino_size_with_border;
    float fetromino_offset = 0.1f * fetromino_size_with_border;
    V2F32 board_world_pos = {
        (world_width - fetromino_size_with_border*10) / 2.0f,
        0.0f
    };


    // background
    V2F32 bg_world_pos = {
        board_world_pos.x,
        board_world_pos.y,
    };
    V2F32 bg_world_dim = {
        fetromino_size_with_border * 10,
        fetromino_size_with_border * 20
    };
    AABB bg_world_aabb = {
        bg_world_pos.x,
        bg_world_pos.y,
        bg_world_pos.x + bg_world_dim.x,
        bg_world_pos.y + bg_world_dim.y,
    };
    Color bg_color = {0.0f, 0.0f, 0.0f, 1.0f};
    g_renderer.PushAABB(bg_world_aabb, bg_color, Fetris::k_z_bg);


    // fetromino parts
    for (size_t y = 0; y < 20; y++) {
        for (size_t x = 0; x < 10; x++) {
            Fetromino::Id fetromino_id = (Fetromino::Id)m_idmap[y][x];
            if (fetromino_id < Fetromino::id_count) {
                V2F32 local_pos = {
                    (float)x * fetromino_size_with_border + fetromino_offset,
                    (float)y * fetromino_size_with_border + fetromino_offset
                };
                V2F32 local_dim = {fetromino_size, fetromino_size};


                V3F32 world_pos = {
                    board_world_pos.x + local_pos.x,
                    board_world_pos.y + local_pos.y,
                    1.0f
                };
                V2F32 world_dim = local_dim;
                AABB world_aabb = {
                    world_pos.x,
                    world_pos.y,
                    world_pos.x + world_dim.x,
                    world_pos.y + world_dim.y,
                };


                Color color = Fetromino::GetColor(fetromino_id);
                g_renderer.PushAABB(world_aabb, color, Fetris::k_z_fetromino);
            }
        }
    }
}

