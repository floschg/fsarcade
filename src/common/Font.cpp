#include "common/Font.hpp"

#include <fstream>
#include <iostream>
#include <cstring>


static inline bool
is_ch_ascii(char32_t ch)
{
    bool result = ch >= ' ' && ch <= '~';
    return result;
}

Font::Font(const char *path, int font_size)
{
    if (!ReadFile(path)) {
        return;
    }


    // init m_file_content, m_font_info
    if (!stbtt_InitFont(&m_font_info, (unsigned char*)m_file_content, 0))
    {
        std::cout << "stbtt_InitFont failed.\n";
        delete[] m_file_content;
        m_file_content = nullptr;
        return;
    }


    // init font settings
    float scale = stbtt_ScaleForPixelHeight(&m_font_info, (float)font_size);
    int baseline, ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&m_font_info, &ascent, &descent, &line_gap);
    baseline = int(scale * (float)-descent);
    ascent   = int(scale * (float)ascent);
    descent  = int(scale * (float)descent);
    line_gap = int(scale * (float)line_gap);

    m_font_scale = scale;
    m_font_baseline = baseline;
    m_font_yadvance = ascent - descent + line_gap;


    // load glyphs
    for (char c = k_first_ascii_ch; c <= k_last_ascii_ch; ++c) {
        InitGlyph(m_glyphs[c-k_first_ascii_ch], static_cast<char32_t>(c));
    }
    memset((void*)&m_fail_glyph, 0, sizeof(m_fail_glyph));
}

Font::~Font()
{
    delete[] m_file_content;
}

bool
Font::ReadFile(const char* path)
{
    std::ifstream file(path, std::ios::in|std::ios::binary|std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streampos end = file.tellg();
    size_t size = static_cast<size_t>(end);
    char *content = new char[size + 1];

    file.seekg(0, std::ios::beg);
    file.read(content, end);
    file.close();
    content[size] = '\0';

    m_file_content = content;
    return m_file_content;
}

void
Font::InitGlyph(Glyph& glyph, char32_t c)
{
    int bbx0, bby0, bbx1, bby1;
    stbtt_GetCodepointBitmapBox(&m_font_info, (int)c, m_font_scale, m_font_scale, &bbx0, &bby0, &bbx1, &bby1);
    int width = bbx1 - bbx0;
    int height = bby1 - bby0;


    size_t size = size_t(width * height);
    uint8_t* bitmap_flipped = new uint8_t[size];
    uint8_t* bitmap_correct = new uint8_t[size];


    stbtt_MakeCodepointBitmap(&m_font_info, bitmap_flipped, width, height, width, m_font_scale, m_font_scale, (int)c);

    uint8_t* dest = bitmap_correct;
    for (int y = 0; y < height; ++y)
    {
        uint8_t* src = bitmap_flipped + size_t((height-1-y)*width);
        for (int x = 0; x < width; ++x)
        {
            *dest++ = *src++;
        }
    }
    delete[] bitmap_flipped;

    glyph.bitmap.w = width;
    glyph.bitmap.h = height;
    glyph.bitmap.pixels = std::unique_ptr<uint8_t>(bitmap_correct);


    int xadvance;
    int left_side_bearing;
    stbtt_GetCodepointHMetrics(&m_font_info, (int)c, &xadvance, &left_side_bearing);
    xadvance          = (int)(m_font_scale * (float)xadvance);
    left_side_bearing = (int)(m_font_scale * (float)left_side_bearing);


    glyph.xoff = static_cast<int>(m_font_scale * (float)bbx0) + left_side_bearing;
    glyph.yoff = -bby1;
    glyph.xadvance = xadvance;

}

AlphaBitmap&
Font::GetAlphaBitmap(char32_t c)
{
    if (m_file_content) {
        if (is_ch_ascii(c)) {
            return m_glyphs[c - k_first_ascii_ch].bitmap;
        }
    }

    return m_fail_glyph.bitmap;
}

Glyph&
Font::GetGlyph(char32_t c)
{
    if (m_file_content) {
        if (is_ch_ascii(c)) {
            return m_glyphs[c - k_first_ascii_ch];
        }
    }
    
    return m_fail_glyph;
}

