#include "animation/demo_sequences.h"
#include <cmath>
#include <iostream>
#include <string>
#include "core/framebuffer.h"
#include "core/mat3.h"
#include "core/scene.h"
#include "core/font.h"
#include "animation/lerp.h"
#include "animation/timeline.h"
#include "animation/transform.h"
#include "animation/exporter.h"
#include "lines/lines.h"
#include "polygons/polygons.h"
#include "clipping/clipping.h"
#include "circles/circles.h"

// ------------------------------------------------------------------
// Shared drawing helpers
// ------------------------------------------------------------------

static void drawRectOutline(Framebuffer& f, int x0, int y0, int x1, int y1, Color c) {
    drawLineBresenham(f, x0, y0, x1, y0, c);
    drawLineBresenham(f, x1, y0, x1, y1, c);
    drawLineBresenham(f, x1, y1, x0, y1, c);
    drawLineBresenham(f, x0, y1, x0, y0, c);
}

static void drawPhoto(Framebuffer& f) {
    for (int y = 80; y < 380; y++)
        for (int x = 80; x < 460; x++)
            f.setPixel(x, y, {(uint8_t)(100 + (x - 80) / 3), (uint8_t)(150 + (y - 80) / 6), 200});
    drawRectOutline(f, 80, 80, 460, 380, {60, 40, 30});
}

// Fill + outline a polygon that has already been transformed by `M`.
static void drawSticker(Framebuffer& f, const Polygon& base, const Mat3& M, Color fill, Color edge) {
    Polygon p = transformPolygon(base, M);
    fillPolygon(f, p, fill);
    for (size_t i = 0; i < p.size(); i++) {
        Vec2 a = p[i], b = p[(i + 1) % p.size()];
        drawLineBresenham(f, (int)a.x, (int)a.y, (int)b.x, (int)b.y, edge);
    }
}

// ------------------------------------------------------------------
// Sequence 1: stickers fly onto the day page  (Transform + lerp + Scene)
// ------------------------------------------------------------------

static void renderStickers(Framebuffer& fb, double t) {
    Scene s;
    s.add("photo", drawPhoto);

    // Star: position, rotation and scale all animate over the whole sequence.
    s.addAnimated("star", [](Framebuffer& f, double t) {
        Transform tr;                                   // pivot (0,0) = star centre
        tr.position = lerp(Vec2{-60, 140}, Vec2{420, 140}, t);
        tr.rotation = lerp(0.0, 375.0, t);              // one spin plus the final 15 degrees
        double sc   = lerp(0.2, 1.2, t);
        tr.scale    = {sc, sc};
        drawSticker(f, makeStar({0, 0}, 50, 22), tr.toMat3(), {255, 200, 0}, {150, 110, 0});
    });

    // Heart: appears at t = 0.25, slides up while its colour fades in.
    s.addTimed("heart", 0.25, 1.0, [](Framebuffer& f, double lt) {
        Transform tr;
        tr.position = lerp(Vec2{170, 700}, Vec2{170, 300}, lt);
        tr.rotation = lerp(0.0, -20.0, lt);
        drawSticker(f, makeHeart({0, 0}, 90), tr.toMat3(),
                    lerp(Color{255, 255, 255}, Color{230, 50, 90}, lt), {120, 20, 50});
    });

    // Square: appears at t = 0.5, rotates and uses Mat3::shear on top of its Transform.
    s.addTimed("square", 0.5, 1.0, [](Framebuffer& f, double lt) {
        Transform tr;
        tr.position = {560, 300};
        tr.rotation = lerp(0.0, 30.0, lt);
        Mat3 M = tr.toMat3() * Mat3::shear(lerp(0.0, 0.35, lt), 0.0);
        drawSticker(f, makeSquare({0, 0}, 70), M, {80, 180, 120}, {20, 80, 50});
    });

    // Circle: grows in from nothing.
    s.addTimed("circle", 0.4, 1.0, [](Framebuffer& f, double lt) {
        fillCircle(f, 620, 150, (int)std::lround(lerp(0.0, 45.0, lt)), {120, 100, 220});
    });

    // Date stamp pops in at the end.
    s.addTimed("date", 0.8, 1.0, [](Framebuffer& f, double) {
        drawText(f, 90, 405, "04-10-2026", 4, {40, 40, 40});
    });

    s.render(fb, t);
}

// ------------------------------------------------------------------
// Sequence 2: calendar highlight glides from one day to another
// (placeholder for the calendar owner's progressive grid + zoom work)
// ------------------------------------------------------------------

static Vec2 calendarCellCentre(int day) {
    const int x0 = 50, y0 = 80, cw = 100, ch = 80, start = 4;
    int col = (start + day - 1) % 7, row = (start + day - 1) / 7;
    return {x0 + col * cw + cw / 2.0, y0 + row * ch + ch / 2.0};
}

static void renderCalendarMove(Framebuffer& fb, double t) {
    const int x0 = 50, y0 = 80, cw = 100, ch = 80;
    Scene s;
    s.addAnimated("highlight", [](Framebuffer& f, double t) {
        Vec2 c = lerp(calendarCellCentre(4), calendarCellCentre(20), t);
        fillPolygon(f, makeSquare(c, 70), lerp(Color{255, 220, 100}, Color{255, 140, 80}, t));
    });
    s.add("grid", [=](Framebuffer& f) {
        Color grid{90, 90, 90};
        for (int i = 0; i <= 7; i++) drawLineBresenham(f, x0 + i * cw, y0, x0 + i * cw, y0 + 5 * ch, grid);
        for (int j = 0; j <= 5; j++) drawLineBresenham(f, x0, y0 + j * ch, x0 + 7 * cw, y0 + j * ch, grid);
        for (int d = 1; d <= 31; d++) {
            Vec2 c = calendarCellCentre(d);
            drawText(f, (int)c.x - cw / 2 + 10, (int)c.y - ch / 2 + 10, std::to_string(d), 4, {30, 30, 30});
        }
    });
    s.render(fb, t);
}

// ------------------------------------------------------------------
// Sequence 3: crossfade from the day page to the calendar
// (placeholder: the transitions owner replaces this with the real crossfade)
// ------------------------------------------------------------------

static void renderCrossfade(Framebuffer& fb, double t) {
    Framebuffer a(fb.width, fb.height, {250, 244, 230});
    Framebuffer b(fb.width, fb.height, {255, 255, 255});
    renderStickers(a, 1.0);
    renderCalendarMove(b, 1.0);
    for (size_t i = 0; i < fb.pixels.size(); i++)
        fb.pixels[i] = lerp(a.pixels[i], b.pixels[i], t);
}

// ------------------------------------------------------------------

void runDemoSequences() {
    Timeline tl;                 // 24 frames at 12 fps
    struct Seq { const char* name; RenderFrameFn fn; Color bg; };
    const Seq seqs[] = {
        {"stickers",  renderStickers,     {250, 244, 230}},
        {"calendar",  renderCalendarMove, {255, 255, 255}},
        {"crossfade", renderCrossfade,    {255, 255, 255}},
    };
    for (const auto& s : seqs) {
        ExportResult r = exportAnimation(s.name, tl, 800, 600, s.bg, s.fn);
        if (r.ok) std::cout << "Wrote " << r.framesWritten << " frames and " << r.contactSheetPath << "\n";
        else      std::cout << "FAILED to export sequence '" << s.name << "'\n";
    }
}