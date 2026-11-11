#include "textures/font_atlas.h"
#include "core/font.h"
#include <cstring>
#include <cctype>

const char* kFontChars = " 0123456789-ABCDEFGHIJKLMNOPQRSTUVWXYZ";

int fontAtlasWidth() { return (int)std::strlen(kFontChars) * kCellW; }

Framebuffer buildFontAtlas() {
    Framebuffer atlas(fontAtlasWidth(), kCellH, {0, 0, 0, 0});     // fully transparent
    for (int i = 1; kFontChars[i]; ++i)                            // index 0 is the blank space
        drawText(atlas, i * kCellW, 0, std::string(1, kFontChars[i]), 1, {255, 255, 255, 255});
    return atlas;
}

int glyphIndex(char c) {
    c = (char)std::toupper((unsigned char)c);
    const char* p = c ? std::strchr(kFontChars, c) : nullptr;
    return p ? (int)(p - kFontChars) : -1;
}

GlyphUV glyphUV(int index) {
    float W = (float)fontAtlasWidth();
    return { index * kCellW / W, 0.0f, (index * kCellW + kGlyphW) / W, 1.0f };
}
