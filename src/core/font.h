#pragma once
#include <string>
#include "core/framebuffer.h"

inline int textWidth(const std::string& text, int scale) {
    if (text.empty() || scale <= 0) return 0;
    return static_cast<int>(text.size()) * 4 * scale - scale;
}

inline void drawText(Framebuffer& fb, int x, int y, const std::string& s, int scale, Color c) {
    static const uint16_t digits[10] =
        {0x7B6F,0x2C97,0x73E7,0x73CF,0x5BC9,0x79CF,0x79EF,0x7249,0x7BEF,0x7BCF};
    // Letters use the same 3x5 grid as the original digits, at the same scale.
    static const uint8_t uppercase[26][5] = {
        {0b010,0b101,0b111,0b101,0b101}, // A
        {0b110,0b101,0b110,0b101,0b110}, // B
        {0b111,0b100,0b100,0b100,0b111}, // C
        {0b110,0b101,0b101,0b101,0b110}, // D
        {0b111,0b100,0b110,0b100,0b111}, // E
        {0b111,0b100,0b110,0b100,0b100}, // F
        {0b111,0b100,0b101,0b101,0b111}, // G
        {0b101,0b101,0b111,0b101,0b101}, // H
        {0b111,0b010,0b010,0b010,0b111}, // I
        {0b001,0b001,0b001,0b101,0b111}, // J
        {0b101,0b101,0b110,0b101,0b101}, // K
        {0b100,0b100,0b100,0b100,0b111}, // L
        {0b101,0b111,0b111,0b101,0b101}, // M
        {0b101,0b111,0b111,0b111,0b101}, // N
        {0b111,0b101,0b101,0b101,0b111}, // O
        {0b110,0b101,0b110,0b100,0b100}, // P
        {0b111,0b101,0b101,0b111,0b001}, // Q
        {0b110,0b101,0b110,0b110,0b101}, // R
        {0b111,0b100,0b111,0b001,0b111}, // S
        {0b111,0b010,0b010,0b010,0b010}, // T
        {0b101,0b101,0b101,0b101,0b111}, // U
        {0b101,0b101,0b101,0b101,0b010}, // V
        {0b101,0b101,0b111,0b111,0b101}, // W
        {0b101,0b101,0b010,0b101,0b101}, // X
        {0b101,0b101,0b010,0b010,0b010}, // Y
        {0b111,0b001,0b010,0b100,0b111}  // Z
    };
    for (char ch : s) {
        uint16_t bits = 0;
        if (ch >= '0' && ch <= '9') bits = digits[ch - '0'];
        else if (ch == '-')         bits = 0x01C0;
        bool isUppercase = ch >= 'A' && ch <= 'Z';
        const int glyphWidth = 3;
        const int glyphHeight = 5;
        for (int row = 0; row < glyphHeight; row++) {
            int rowBits = isUppercase ? uppercase[ch - 'A'][row] : (bits >> (12 - row * 3)) & 7;
            for (int column = 0; column < glyphWidth; column++)
                if (rowBits & (1 << (glyphWidth - 1 - column)))
                    for (int dy = 0; dy < scale; dy++) for (int dx = 0; dx < scale; dx++)
                        fb.setPixel(x + column * scale + dx, y + row * scale + dy, c);
        }
        x += (glyphWidth + 1) * scale;
    }
}