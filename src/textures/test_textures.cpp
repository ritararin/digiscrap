// CPU-only tests for Option 3: loaders, font atlas, shader-filter mirror.
#include <iostream>
#include <cmath>
#include <cstdio>
#include "textures/image_loader.h"
#include "textures/font_atlas.h"
#include "textures/filter_mirror.h"
#include "colour/colour.h"
#include "core/font.h"

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::cout << "FAIL: " << msg << "\n"; ++failures; } } while (0)
static bool near(Color a, Color b, int tol) {
    return std::abs(a.r - b.r) <= tol && std::abs(a.g - b.g) <= tol && std::abs(a.b - b.b) <= tol;
}

int main() {
    // ---- BMP round trip (our saveBMP -> our loadBMP), odd width exercises row padding ----
    Framebuffer src(5, 3, {0, 0, 0});
    for (int y = 0; y < 3; ++y) for (int x = 0; x < 5; ++x)
        src.pixels[y * 5 + x] = { (uint8_t)(x * 50), (uint8_t)(y * 100), (uint8_t)(x * 10 + y), 255 };
    CHECK(src.saveBMP("test_tex.bmp"), "save bmp");
    Framebuffer back(1, 1);
    CHECK(loadBMP("test_tex.bmp", back), "load bmp");
    CHECK(back.width == 5 && back.height == 3, "bmp size");
    bool same = back.width == 5 && back.height == 3;
    for (int i = 0; same && i < 15; ++i)
        same = near(back.pixels[i], src.pixels[i], 0);
    CHECK(same, "bmp pixels identical (bottom-up rows handled)");
    std::remove("test_tex.bmp");

    // ---- PPM round trip ----
    CHECK(src.savePPM("test_tex.ppm"), "save ppm");
    Framebuffer p(1, 1);
    CHECK(loadPPM("test_tex.ppm", p), "load ppm");
    same = p.width == 5 && p.height == 3;
    for (int i = 0; same && i < 15; ++i) same = near(p.pixels[i], src.pixels[i], 0);
    CHECK(same, "ppm pixels identical");
    std::remove("test_tex.ppm");

    // ---- bad input is rejected, output untouched ----
    Framebuffer keep(2, 2, {9, 9, 9});
    CHECK(!loadBMP("does_not_exist.bmp", keep), "missing file rejected");
    CHECK(!loadImage("photo.jpg", keep), "unsupported extension rejected");
    CHECK(keep.width == 2 && keep.pixels[0].r == 9, "failed load leaves output unchanged");

    // ---- resize keeps a flat colour flat and hits the requested size ----
    Framebuffer flat(4, 4, {100, 150, 200});
    Framebuffer big = resizeBilinear(flat, 9, 7);
    CHECK(big.width == 9 && big.height == 7 && near(big.pixels[30], {100, 150, 200}, 0), "bilinear resize");

    // ---- font atlas matches drawText pixel-for-pixel ----
    Framebuffer atlas = buildFontAtlas();
    CHECK(atlas.width == fontAtlasWidth() && atlas.height == kCellH, "atlas size");
    bool glyphsOk = true;
    for (const char* c = kFontChars + 1; *c; ++c) {
        Framebuffer one(kCellW, kCellH, {0, 0, 0, 0});
        drawText(one, 0, 0, std::string(1, *c), 1, {255, 255, 255, 255});
        int gi = glyphIndex(*c);
        for (int y = 0; y < kCellH; ++y) for (int x = 0; x < kCellW; ++x)
            if (one.pixels[y * kCellW + x].a != atlas.pixels[y * atlas.width + gi * kCellW + x].a) glyphsOk = false;
    }
    CHECK(glyphsOk, "every atlas glyph equals drawText output");
    CHECK(glyphIndex('a') == glyphIndex('A') && glyphIndex('#') == -1, "glyphIndex mapping");
    GlyphUV uv = glyphUV(glyphIndex('7'));
    CHECK(uv.u0 < uv.u1 && uv.u1 <= 1.0f, "glyph uv range");

    // ---- shader maths mirror vs C++ reference filters (strength 1) ----
    Color samples[] = {{0,0,0}, {255,255,255}, {200,120,40}, {10,200,90}, {128,128,128}, {255,0,0}};
    for (Color c : samples) {
        FilterParams f;
        f.strength = 1.0f;
        f.kind = FILTER_GRAYSCALE; CHECK(near(applyFilterMirror(c, f), grayscale(c), 1), "grayscale mirror");
        f.kind = FILTER_SEPIA;     CHECK(near(applyFilterMirror(c, f), sepia(c), 1), "sepia mirror");
        f.kind = FILTER_INVERT;    CHECK(near(applyFilterMirror(c, f), invert(c), 1), "invert mirror");
        f.kind = FILTER_CONTRAST; f.amount = 1.5f;
        CHECK(near(applyFilterMirror(c, f), contrast(c, 1.5), 1), "contrast mirror");
        f.kind = FILTER_BRIGHTNESS; f.amount = 40 / 255.0f;
        CHECK(near(applyFilterMirror(c, f), brightness(c, 40), 1), "brightness mirror");
        f.kind = FILTER_TINT; f.tint = {255, 140, 180, 255};
        CHECK(near(applyFilterMirror(c, f), tint(c, f.tint, 1.0), 1), "tint mirror");
        // strength 0 is always the original, strength 0.5 is halfway
        f.kind = FILTER_SEPIA; f.strength = 0.0f;
        CHECK(near(applyFilterMirror(c, f), c, 0), "strength 0 = original");
        f.strength = 0.5f;
        Color half = applyFilterMirror(c, f), full = sepia(c);
        CHECK(near(half, {(uint8_t)((c.r + full.r) / 2), (uint8_t)((c.g + full.g) / 2), (uint8_t)((c.b + full.b) / 2)}, 2), "strength 0.5 = halfway");
    }
    CHECK(applyFilterMirror({1, 2, 3, 77}, FilterParams()).a == 77, "alpha preserved");

    std::cout << (failures ? "textures tests FAILED\n" : "textures tests passed\n");
    return failures ? 1 : 0;
}
