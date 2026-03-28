#pragma once

#include <stb_truetype.h>

#include <memory>


struct AlphaBitmap {
    int32_t w;
    int32_t h;
    std::unique_ptr<uint8_t> pixels;
};


struct Glyph {
    int32_t xoff;
    int32_t yoff;
    int32_t xadvance;
    AlphaBitmap bitmap;
};


class Font {
public:
    explicit Font(const char* path, int size);
    ~Font();

    AlphaBitmap& GetAlphaBitmap(char32_t c);
    Glyph& GetGlyph(char32_t c);



private:
    bool ReadFile(const char* path);
    void InitGlyph(Glyph& glyph, char32_t c);


private:
    static constexpr char k_first_ascii_ch = ' ';
    static constexpr char k_last_ascii_ch = '~';
    static constexpr char k_ascii_glyph_count = k_last_ascii_ch - k_first_ascii_ch + 1;


    const char* m_file_content = nullptr;
    stbtt_fontinfo m_font_info;

    float m_font_scale;
    int m_font_baseline;
    int m_font_yadvance;

    Glyph m_glyphs[k_ascii_glyph_count];
    Glyph m_fail_glyph;
};


