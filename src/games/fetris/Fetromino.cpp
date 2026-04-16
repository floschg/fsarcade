#include "games/fetris/Fetromino.hpp"
#include "games/fetris/Fetris.hpp"
#include "renderer/Renderer.hpp"

#include <random>

// layout of a left_aligned_bitmap: xxxx000000000000
// layout of a board_bitmap is 111xxxxxxxxxx111
static const uint16_t k_left_aligned_bitmaps[7][4][4] = {
    {
        // O piece
        {0x6000, 0x6000, 0x0000, 0x0000}, // orientation 0
        {0x6000, 0x6000, 0x0000, 0x0000}, // orientation 1
        {0x6000, 0x6000, 0x0000, 0x0000}, // orientation 2
        {0x6000, 0x6000, 0x0000, 0x0000}, // orientation 3
    },
    {
        // S piece
        {0x6000, 0x3000, 0x0000, 0x0000}, // orientation 0
        {0x1000, 0x3000, 0x2000, 0x0000}, // orientation 1
        {0x6000, 0x3000, 0x0000, 0x0000}, // orientation 2
        {0x1000, 0x3000, 0x2000, 0x0000}, // orientation 3
    },
    {
        // Z piece
        {0x3000, 0x6000, 0x0000, 0x0000}, // orientation 0
        {0x2000, 0x3000, 0x1000, 0x0000}, // orientation 1
        {0x3000, 0x6000, 0x0000, 0x0000}, // orientation 2
        {0x2000, 0x3000, 0x1000, 0x0000}, // orientation 3
    },
    {
        // T piece
        {0x2000, 0x7000, 0x0000, 0x0000}, // orientation 0
        {0x2000, 0x6000, 0x2000, 0x0000}, // orientation 1
        {0x0000, 0x7000, 0x2000, 0x0000}, // orientation 2
        {0x2000, 0x3000, 0x2000, 0x0000}, // orientation 3
    },
    {
        // L piece
        {0x4000, 0x7000, 0x0000, 0x0000}, // orientation 0
        {0x2000, 0x2000, 0x6000, 0x0000}, // orientation 1
        {0x0000, 0x7000, 0x1000, 0x0000}, // orientation 2
        {0x3000, 0x2000, 0x2000, 0x0000}, // orientation 3
    },
    {
        // J piece
        {0x1000, 0x7000, 0x0000, 0x0000}, // orientation 0
        {0x6000, 0x2000, 0x2000, 0x0000}, // orientation 1
        {0x0000, 0x7000, 0x4000, 0x0000}, // orientation 2
        {0x2000, 0x2000, 0x3000, 0x0000}, // orientation 3
    },
    {
        // I piece
        {0x0000, 0xf000, 0x0000, 0x0000}, // orientation 0
        {0x2000, 0x2000, 0x2000, 0x2000}, // orientation 1
        {0x0000, 0xf000, 0x0000, 0x0000}, // orientation 2
        {0x2000, 0x2000, 0x2000, 0x2000}, // orientation 3
    }
};

Fetromino::Fetromino(uint16_t* board_bitmap)
    : m_board_bitmap{board_bitmap}
{
}

Fetromino::Id
Fetromino::GenerateRandomId()
{
    static std::uniform_int_distribution<int> s_dist(0, id_count-1);
    static std::mt19937 s_rng((std::random_device()()));
    Id id = static_cast<Id>(s_dist(s_rng));
    return id;
}


void
Fetromino::Reset(Id id)
{
    m_id = id;
    m_pos = {6, 20};
    m_ori = {0};
}

Fetromino::Id
Fetromino::GetId()
{
    return m_id;
}

BoardPos
Fetromino::GetPos()
{
    return m_pos;
}

void
Fetromino::GetBitmap(uint16_t* bitmap)
{
    GetBitmap(m_id, m_pos, m_ori, bitmap);
}

bool
Fetromino::IsCollisionWithBoard()
{
    bool is_collision = IsCollisionWithBoard(m_id, m_pos, m_ori, m_board_bitmap);
    return is_collision;
}

void
Fetromino::MaybeRotate(Rotation rotation)
{
    int32_t ori = (m_ori + rotation) % 4;
    if (!IsCollisionWithBoard(m_id, m_pos, ori, m_board_bitmap)) {
        m_ori = ori;
    }
}

void
Fetromino::MaybeMoveHorizontally(Direction direction)
{
    BoardPos pos = m_pos;
    pos.x += static_cast<int32_t>(direction);
    if (!IsCollisionWithBoard(m_id, pos, m_ori, m_board_bitmap)) {
        m_pos.x = pos.x;
    }
}

bool
Fetromino::MaybeMoveDown()
{
    BoardPos pos = m_pos;
    pos.y -= 1;
    if (!IsCollisionWithBoard(m_id, pos, m_ori, m_board_bitmap)) {
        m_pos.y = pos.y;
        return true;
    }
    return false;
}

void
Fetromino::Draw()
{
    float world_width = 4.0f;
    float world_height = 3.0f;
    float fetromino_size_with_border = world_height / 20.0f;

    float x0 = static_cast<float>(m_pos.x - 3);
    float y0 = static_cast<float>(m_pos.y - 2);

    V2F32 world_pos = {
        ((world_width - fetromino_size_with_border*10) / 2.0f) + x0 * fetromino_size_with_border,
        y0 * fetromino_size_with_border
    };

    Fetromino::Draw(m_id, m_ori, world_pos, 1.0f);
}

bool
Fetromino::IsCollisionWithBoard(Id id, BoardPos pos, int32_t ori, uint16_t *board_bitmap)
{
    uint16_t fetromino_bitmap[16];
    GetBitmap(id, pos, ori, fetromino_bitmap);

    uint64_t fetromino_bits = *(uint64_t*)(fetromino_bitmap);
    uint64_t board_bits = *(uint64_t*)(&board_bitmap[pos.y]);
    bool is_collision = fetromino_bits & board_bits;
    return is_collision;
}

void
Fetromino::GetBitmap(Id id, BoardPos pos, int32_t ori, uint16_t *bitmap)
{
    uint64_t *src  = (uint64_t*)k_left_aligned_bitmaps[id][ori];
    uint64_t *dest = (uint64_t*)bitmap;
    *dest = *src >> pos.x;
}

Color
Fetromino::GetColor(Id id)
{
    Color color;

    switch (id) {
    case i_piece:
    case o_piece:
    case t_piece: {
        color = {0.8f, 0.8f, 0.8f, 1.0f};
    } break;

    case j_piece:
    case s_piece: {
        color = {0.8f, 0.2f, 0.2f, 1.0f};
    } break;

    default: {
        color = {0.2f, 0.4f, 0.2f, 1.0f};
    }
    }

    return color;
}

void
Fetromino::Draw(Id id, int32_t ori, V2F32 pos, float scale)
{
    float world_height = 3.0f;
    float fetromino_size_with_border = scale * world_height / 20.0f;
    float fetromino_size = 0.8f * fetromino_size_with_border;
    float fetromino_offset = 0.1f * fetromino_size_with_border;

    uint16_t *left_aligned_bitmap = (uint16_t*)k_left_aligned_bitmaps[id][ori];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (left_aligned_bitmap[y] & (0x8000 >> x)) {
                V2F32 local_pos = {
                    (float)x * fetromino_size_with_border + fetromino_offset,
                    (float)y * fetromino_size_with_border + fetromino_offset
                };
                V2F32 local_dim = {fetromino_size, fetromino_size};


                V2F32 world_pos = {
                    pos.x + local_pos.x,
                    pos.y + local_pos.y,
                };
                V2F32 world_dim = local_dim;
                AABB world_aabb = {
                    world_pos.x,
                    world_pos.y,
                    world_pos.x + world_dim.x,
                    world_pos.y + world_dim.y,
                };


                Color color = GetColor(id);
                g_renderer.PushAABB(world_aabb, color, Fetris::k_z_fetromino);
            }
        }
    }
}

