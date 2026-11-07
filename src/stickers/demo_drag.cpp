#include "stickers/demo_drag.h"
#include <cmath>
#include <iostream>
#include "core/framebuffer.h"
#include "core/mat3.h"
#include "core/scene.h"
#include "animation/lerp.h"
#include "animation/timeline.h"
#include "animation/transform.h"
#include "animation/exporter.h"
#include "stickers/sticker.h"
#include "stickers/morph.h"
#include "lines/lines.h"
#include "polygons/polygons.h"
#include "circles/circles.h"

// ------------------------------------------------------------------
// Palette
// ------------------------------------------------------------------
static const Color gold      {255, 200,   0};
static const Color darkGold  {150, 110,   0};
static const Color pink      {230,  50,  90};
static const Color darkPink  {120,  20,  50};
static const Color teal      { 80, 180, 170};
static const Color darkTeal  { 30,  90,  80};
static const Color green     { 80, 180, 120};
static const Color darkGreen { 20,  80,  50};
static const Color cursorRed {255,  80,  80};
static const Color bg        {250, 244, 230};

// ------------------------------------------------------------------
// Sequence 1 – Scripted drag simulation
//
//   t  0.00–0.15  rest (all stickers idle)
//   t  0.15–0.45  drag   — star moves  (200,300) → (500,200)
//   t  0.45–0.60  resize — star scales  1.0 → 1.6
//   t  0.60–0.75  rotate — star rotates 0° → 35°
//   t  0.75–0.85  bring-to-front  (z jumps to max+1)
//   t  0.85–1.00  settle (all idle, final pose)
// ------------------------------------------------------------------

static void renderDragDemo(Framebuffer& fb, double t) {
    // Build four overlapping stickers.
    Sticker star  = makeSticker(makeStar({0,0}, 50, 22),               {200, 300}, gold,  darkGold,  1);
    Sticker heart = makeSticker(makeHeart({0,0}, 90),                  {350, 280}, pink,  darkPink,  2);
    Sticker hex   = makeSticker(makeRegularPolygon({0,0}, 45, 6),      {300, 350}, teal,  darkTeal,  3);
    Sticker sq    = makeSticker(makeSquare({0,0}, 70),                 {450, 320}, green, darkGreen, 0);

    // ---- animate the star ----
    double dragT   = segmentT(t, 0.15, 0.45);
    double resizeT = segmentT(t, 0.45, 0.60);
    double rotateT = segmentT(t, 0.60, 0.75);

    star.xform.position = lerp(Vec2{200, 300}, Vec2{500, 200}, dragT);
    double sc = lerp(1.0, 1.6, resizeT);
    star.xform.scale = {sc, sc};
    star.xform.rotation = lerp(0.0, 35.0, rotateT);
    if (t >= 0.75) star.zOrder = 10;          // bring to front

    // ---- assemble the scene ----
    StickerScene scene;
    scene.add(sq);
    scene.add(star);
    scene.add(heart);
    scene.add(hex);
    scene.sortByZ();
    scene.render(fb);

    // ---- draw simulated cursor ----
    Vec2 cursor = star.xform.position;
    fillCircle(fb, (int)cursor.x, (int)cursor.y, 5, cursorRed);
}

// ------------------------------------------------------------------
// Sequence 2 – Morph  (star ↔ regular decagon, same 10 vertices)
// ------------------------------------------------------------------

static void renderMorphDemo(Framebuffer& fb, double t) {
    Polygon star    = makeStar({0, 0}, 60, 26);          // 10 vertices
    Polygon decagon = makeRegularPolygon({0, 0}, 60, 10); // 10 vertices

    // Morph forward in the first half, backward in the second (ping-pong).
    double mt = t <= 0.5 ? t * 2.0 : (1.0 - t) * 2.0;
    Polygon shape = morphPolygon(star, decagon, mt);

    // Place it at screen centre with a gentle spin.
    Transform xf;
    xf.pivot    = {0, 0};
    xf.position = {400, 300};
    xf.rotation = lerp(0.0, 360.0, t);
    double sc   = lerp(1.0, 1.4, mt);
    xf.scale    = {sc, sc};

    Polygon wp = xf.apply(shape);
    fillPolygon(fb, wp, lerp(gold, teal, mt));
    for (size_t i = 0; i < wp.size(); i++) {
        Vec2 a = wp[i], b = wp[(i + 1) % wp.size()];
        drawLineBresenham(fb, (int)a.x, (int)a.y, (int)b.x, (int)b.y, darkGold);
    }
}

// ------------------------------------------------------------------
// Entry point – exports both sequences
// ------------------------------------------------------------------

void runStickerDemos() {
    Timeline tl;                             // 24 frames at 12 fps

    struct Seq { const char* name; RenderFrameFn fn; Color bg; };
    const Seq seqs[] = {
        {"drag_demo",  renderDragDemo,  bg},
        {"morph_demo", renderMorphDemo, bg},
    };

    for (const auto& s : seqs) {
        ExportResult r = exportAnimation(s.name, tl, 800, 600, s.bg, s.fn);
        if (r.ok)
            std::cout << "Wrote " << r.framesWritten
                      << " frames and " << r.contactSheetPath << "\n";
        else
            std::cout << "FAILED to export '" << s.name << "'\n";
    }
}

