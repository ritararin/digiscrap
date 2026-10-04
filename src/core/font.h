#pragma once
#include <string>
#include "core/framebuffer.h"

inline void drawText(Framebuffer& fb, int x, int y, const std::string& s, int scale, Color c) {
    static const uint16_t digits[10] =
        {0x7B6F,0x2C97,0x73E7,0x73CF,0x5BC9,0x79CF,0x79EF,0x7249,0x7BEF,0x7BCF};
    for (char ch : s) {
        uint16_t bits = 0;
        if (ch >= '0' && ch <= '9') bits = digits[ch - '0'];
        else if (ch == '-')         bits = 0x01C0;
        for (int r = 0; r < 5; r++) for (int col = 0; col < 3; col++)
            if (bits & (1 << (14 - (r*3 + col))))
                for (int dy = 0; dy < scale; dy++) for (int dx = 0; dx < scale; dx++)
                    fb.setPixel(x + col*scale + dx, y + r*scale + dy, c);
        x += 4 * scale;
    }
}