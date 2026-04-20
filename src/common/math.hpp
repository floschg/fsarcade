#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cassert>


struct V2ST {
    size_t x;
    size_t y;

    bool operator==(V2ST &b);
    bool operator==(const V2ST& other) const {
        return x == other.x && y == other.y;
    }
};

struct V2F32 {
    float x;
    float y;

    V2F32 operator/(float scalar);
    V2F32 operator*(float scalar);
    V2F32 operator+(V2F32 other);
};

struct V3F32 {
    float x;
    float y;
    float z;

    V3F32 operator/(float scalar);
    V3F32 operator*(float scalar);
};

struct V4F32 {
    float x;
    float y;
    float z;
    float w;

    V4F32 operator/(float scalar);
    V4F32 operator*(float scalar);
};

struct V2I32 {
    int32_t x;
    int32_t y;

    bool operator==(V2I32 other);
};

typedef float Mat4x4[4][4];

inline V2F32
mat4x4_dot_v2f32(Mat4x4 mat, V2F32* pos)
{
    float x = pos->x;
    float y = pos->y;

    V2F32 result;
    result.x = mat[0][0]*x + mat[0][1]*y + mat[0][2]*0.0f + mat[0][3]*1.0f;
    result.y = mat[1][0]*x + mat[1][1]*y + mat[1][2]*0.0f + mat[1][3]*1.0f;
    return result;
};

inline void
mat4x4_dot_mat4x4(Mat4x4 a, Mat4x4 b, Mat4x4 c)
{
    c[0][0] = a[0][0]*b[0][0] + a[0][1]*b[1][0] + a[0][2]*b[2][0] + a[0][3]*b[3][0];
    c[0][1] = a[0][0]*b[0][1] + a[0][1]*b[1][1] + a[0][2]*b[2][1] + a[0][3]*b[3][1];
    c[0][2] = a[0][0]*b[0][2] + a[0][1]*b[1][2] + a[0][2]*b[2][2] + a[0][3]*b[3][2];
    c[0][3] = a[0][0]*b[0][3] + a[0][1]*b[1][3] + a[0][2]*b[2][3] + a[0][3]*b[3][3];

    c[1][0] = a[1][0]*b[0][0] + a[1][1]*b[1][0] + a[1][2]*b[2][0] + a[1][3]*b[3][0];
    c[1][1] = a[1][0]*b[0][1] + a[1][1]*b[1][1] + a[1][2]*b[2][1] + a[1][3]*b[3][1];
    c[1][2] = a[1][0]*b[0][2] + a[1][1]*b[1][2] + a[1][2]*b[2][2] + a[1][3]*b[3][2];
    c[1][3] = a[1][0]*b[0][3] + a[1][1]*b[1][3] + a[1][2]*b[2][3] + a[1][3]*b[3][3];

    c[2][0] = a[2][0]*b[0][0] + a[2][1]*b[1][0] + a[2][2]*b[2][0] + a[2][3]*b[3][0];
    c[2][1] = a[2][0]*b[0][1] + a[2][1]*b[1][1] + a[2][2]*b[2][1] + a[2][3]*b[3][1];
    c[2][2] = a[2][0]*b[0][2] + a[2][1]*b[1][2] + a[2][2]*b[2][2] + a[2][3]*b[3][2];
    c[2][3] = a[2][0]*b[0][3] + a[2][1]*b[1][3] + a[2][2]*b[2][3] + a[2][3]*b[3][3];

    c[3][0] = a[3][0]*b[0][0] + a[3][1]*b[1][0] + a[3][2]*b[2][0] + a[3][3]*b[3][0];
    c[3][1] = a[3][0]*b[0][1] + a[3][1]*b[1][1] + a[3][2]*b[2][1] + a[3][3]*b[3][1];
    c[3][2] = a[3][0]*b[0][2] + a[3][1]*b[1][2] + a[3][2]*b[2][2] + a[3][3]*b[3][2];
    c[3][3] = a[3][0]*b[0][3] + a[3][1]*b[1][3] + a[3][2]*b[2][3] + a[3][3]*b[3][3];
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

struct AABB {
    float x0;
    float y0;
    float x1;
    float y1;
};

struct Circle {
    float x;
    float y;
    float r;
};


inline bool
Intersect_AABB_Circle(AABB aabb, Circle circle)
{
    assert(aabb.x0 <= aabb.x1);
    assert(aabb.y0 <= aabb.y1);

    float closest_x = std::max(aabb.x0, std::min(circle.x, aabb.x1));
    float closest_y = std::max(aabb.y0, std::min(circle.y, aabb.y1));

    float dx = closest_x - circle.x;
    float dy = closest_y - circle.y; 
    float d_sq = dx*dx + dy*dy;

    float r_sq = circle.r * circle.r;

    bool is_intersect = d_sq <= r_sq;
    return is_intersect;
}

