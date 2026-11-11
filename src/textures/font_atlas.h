#pragma once
#include <string>
#include "core/framebuffer.h"

// Bitmap font texture built from our own 3x5 font (core/font.h).
// Every glyph sits in a cell of kCellW x kCellH pixels (3x5 glyph + 1px gap),
// laid out left to right in the order of kFontChars. Glyph pixels are white
// with alpha 255; everything else is transparent, so a shader can tint it.
extern const char* kFontChars;     // " 0123456789-ABCDEFGHIJKLMNOPQRSTUVWXYZ"
constexpr int kCellW = 4, kCellH = 5, kGlyphW = 3;

Framebuffer buildFontAtlas();
int glyphIndex(char c);            // -1 if the character has no glyph (lowercase is upper-cased)
int fontAtlasWidth();

struct GlyphUV { float u0, v0, u1, v1; };
GlyphUV glyphUV(int index);        // texture rectangle of the 3x5 glyph pixels
