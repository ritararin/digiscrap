#include <iostream>
#include <string>
#include <filesystem>
#include "core/framebuffer.h"
#include "core/mat3.h"
#include "core/scene.h"
#include "core/font.h"
#include "lines/lines.h"
#include "polygons/polygons.h"
#include "clipping/clipping.h"
#include "colour/colour.h"
#include "circles/circles.h"
#include "curves/curves.h"

// ---------- shared palette (taken from the project deck) ----------
namespace pal {
    const Color brown     {154, 106,  67};
    const Color dark      { 77,  74,  63};
    const Color cream     {249, 228, 192};
    const Color paper     {252, 241, 217};
    const Color white     {255, 252, 245};
    const Color mustard   {228, 165,  69};
    const Color honey     {249, 205, 125};
    const Color green     {110, 145, 121};
    const Color deepGreen { 84, 118,  96};
    const Color sage      {173, 170, 134};
    const Color terracotta{196,  92,  70};
    const Color shadow    {214, 186, 140};   // soft shadow on cream paper
    const Color tapeA     {228, 165,  69, 200};
    const Color tapeB     {110, 145, 121, 200};
}
static const bool kDebug = false;   // true = show selection boxes and clip window

// ---------- helpers ----------
static Polygon rectPoly(double x0, double y0, double x1, double y1) {
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
}
static void fillRect(Framebuffer& f, double x0, double y0, double x1, double y1, Color c) {
    fillPolygon(f, rectPoly(x0, y0, x1, y1), c);
}
static void drawOutline(Framebuffer& f, const Polygon& p, Color c) {
    for (size_t i = 0; i < p.size(); i++) {
        Vec2 a = p[i], b = p[(i + 1) % p.size()];
        drawLineBresenham(f, (int)a.x, (int)a.y, (int)b.x, (int)b.y, c);
    }
}
static void drawRect(Framebuffer& f, int x0, int y0, int x1, int y1, Color c) {
    drawLineBresenham(f, x0, y0, x1, y0, c);
    drawLineBresenham(f, x1, y0, x1, y1, c);
    drawLineBresenham(f, x1, y1, x0, y1, c);
    drawLineBresenham(f, x0, y1, x0, y0, c);
}
// Rotate (degrees) and scale about the polygon's own centre, then move it
static Polygon placed(const Polygon& p, double deg, double s, Vec2 move) {
    return transformPolygon(p, Mat3::about(polygonCentroid(p), deg * M_PI / 180.0, s, move));
}
// Offset copy of a polygon, used as a drop shadow
static void dropShadow(Framebuffer& f, Polygon p, double dx = 5, double dy = 6) {
    for (Vec2& v : p) { v.x += dx; v.y += dy; }
    fillPolygon(f, p, pal::shadow);
}
// Every sticker goes through this so they all look the same style
static void sticker(Framebuffer& f, const Polygon& p, Color fill, Color edge) {
    dropShadow(f, p);
    fillPolygon(f, p, fill);
    drawOutline(f, p, edge);
}
// Draw with the team's existing rasterisers, then blend the temporary layer.
// This keeps setPixel's replacement semantics unchanged for other modules.
static void blendLayer(Framebuffer& destination, const std::function<void(Framebuffer&)>& draw) {
    Framebuffer layer(destination.width, destination.height, {0, 0, 0, 0});
    draw(layer);
    for (size_t index = 0; index < layer.pixels.size(); index++)
        if (layer.pixels[index].a != 0)
            destination.pixels[index] = alphaBlend(layer.pixels[index], destination.pixels[index]);
}
// Measure the font's letter/digit widths, including character spacing.
static int textW(const std::string& s, int scale) { return textWidth(s, scale); }

// ---------- day page ----------
void buildDayPage(Framebuffer& fb) {
    Scene s;

    // 1. Notebook background and paper card
    s.add("background", [](Framebuffer& f) {
        f.clear(pal::brown);
        for (int y = 60; y < 580; y += 70)                   // binder holes
            fillCircle(f, 28, y, 11, pal::dark);
        Polygon card = rectPoly(70, 30, 770, 570);
        dropShadow(f, card, 6, 7);
        fillPolygon(f, card, pal::cream);
    });

    // 2. Polaroid with a procedural photo
    s.add("polaroid", [](Framebuffer& f) {
        Polygon frame = rectPoly(120, 80, 480, 430);
        dropShadow(f, frame);
        fillPolygon(f, frame, pal::white);

        for (int y = 100; y < 360; y++)                      // warm green -> mustard gradient
            for (int x = 140; x < 460; x++) {
                double t = ((x - 140) + (y - 100)) / 580.0;
                f.setPixel(x, y, {(uint8_t)(110 + t * 118), (uint8_t)(145 + t * 20),
                                  (uint8_t)(121 - t * 52)});
            }
        fillCircle(f, 395, 160, 30, pal::honey);             // sun
        fillPolygon(f, {{140,360},{140,295},{225,250},{320,305},{395,265},{460,300},{460,360}},
                    pal::deepGreen);                         // hills (concave scanline fill)
        applyFilter(f, 140, 100, 460, 360, sepia);
        drawRect(f, 140, 100, 460, 360, pal::dark);
    });

    // 3. Washi tape: source-over blending reveals the photo and frame beneath.
    s.add("tape", [](Framebuffer& f) {
        blendLayer(f, [](Framebuffer& layer) {
            fillPolygon(layer, placed(rectPoly(-45,-13,45,13), -35, 1.0, {140, 92}), pal::tapeA);
        });
        blendLayer(f, [](Framebuffer& layer) {
            fillPolygon(layer, placed(rectPoly(-45,-13,45,13), 35, 1.0, {462, 92}), pal::tapeB);
        });
    });

    // 4. Stickers
    s.add("star", [](Framebuffer& f) {
        sticker(f, placed(makeStar({0,0}, 50, 22), 15, 1.2, {490, 125}),
                pal::mustard, {150, 100, 30});
    });
    s.add("heart", [](Framebuffer& f) {
        Polygon p = placed(makeHeart({0,0}, 90), -20, 1.0, {200, 320});
        sticker(f, p, pal::terracotta, {120, 50, 35});
        if (kDebug) {
            Vec2 mn, mx; boundingBox(p, mn, mx);
            drawRect(f, (int)mn.x - 4, (int)mn.y - 4, (int)mx.x + 4, (int)mx.y + 4, pal::dark);
            std::cout << "pointInPolygon(heart, centre) = " << pointInPolygon(p, polygonCentroid(p))
                      << " (expect 1)\npointInPolygon(heart, far) = " << pointInPolygon(p, {700, 50})
                      << " (expect 0)\n";
        }
    });
    s.add("hexagon", [](Framebuffer& f) {
        sticker(f, placed(makeRegularPolygon({0,0}, 40, 6), 10, 1.0, {610, 130}),
                pal::sage, pal::dark);
    });
    s.add("square", [](Framebuffer& f) {
        sticker(f, placed(makeSquare({0,0}, 64), 20, 1.0, {560, 290}),
                tint(pal::honey, pal::green, 0.55), {170, 120, 40});
    });
    s.add("circle", [](Framebuffer& f) {
        fillCircle(f, 703, 246, 36, pal::shadow);            // shadow
        fillCircle(f, 698, 240, 36, pal::green);
        drawCircleMidpoint(f, 698, 240, 36, pal::deepGreen);
    });

    // 5. Date stamp (notched polygon + text)
    s.add("date", [](Framebuffer& f) {
        Polygon stamp = makeDateStamp({620, 400}, 280, 64, 12);
        sticker(f, stamp, pal::white, pal::brown);
        std::string d = "05-10-2026";
        drawText(f, 620 - textW(d, 4) / 2, 390, d, 4, pal::dark);
    });

    // 6. Sticker clipped by the card edge, so it peeks in from the corner
    s.add("clipped", [](Framebuffer& f) {
        Polygon big = placed(makeStar({0,0}, 85, 38), 8, 1.0, {735, 540});
        Polygon cut = clipPolygon(big, 70, 30, 770, 570);
        fillPolygon(f, cut, pal::terracotta);
        drawOutline(f, cut, {120, 50, 35});
        if (kDebug) drawRect(f, 70, 30, 770, 570, {90, 90, 90});
    });

    s.render(fb);
}

// ---------- calendar ----------
void buildCalendar(Framebuffer& fb) {
    const int x0 = 70, y0 = 160, cw = 94, ch = 68, hdr = 34;
    const int startCol = 4;      // 1 Oct 2026 is a Thursday (Sunday = column 0)
    const int today = 5;

    fb.clear(pal::brown);
    for (int y = 60; y < 580; y += 70) fillCircle(fb, 28, y, 11, pal::dark);
    Polygon card = rectPoly(55, 30, 770, 570);
    dropShadow(fb, card, 6, 7);
    fillPolygon(fb, card, pal::cream);

    // Title and weekday header
    std::string title = "2026";
    drawText(fb, 412 - textW(title, 5) / 2, 70, title, 5, pal::dark);
    const char* days[7] = {"S", "M", "T", "W", "T", "F", "S"};
    for (int c = 0; c < 7; c++) {
        fillRect(fb, x0 + c * cw, y0 - hdr, x0 + (c + 1) * cw, y0, pal::green);
        drawText(fb, x0 + c * cw + cw / 2 - 10, y0 - hdr + 7, days[c], 3, pal::paper);
    }

    // Cell backgrounds (weekends tinted)
    for (int r = 0; r < 5; r++)
        for (int c = 0; c < 7; c++)
            fillRect(fb, x0 + c * cw, y0 + r * ch, x0 + (c + 1) * cw, y0 + (r + 1) * ch,
                     (c == 0 || c == 6) ? Color{247, 222, 178} : pal::paper);

    // Grid lines (Bresenham)
    for (int i = 0; i <= 7; i++)
        drawLineBresenham(fb, x0 + i * cw, y0 - hdr, x0 + i * cw, y0 + 5 * ch, pal::brown);
    for (int j = 0; j <= 5; j++)
        drawLineBresenham(fb, x0, y0 + j * ch, x0 + 7 * cw, y0 + j * ch, pal::brown);
    drawLineBresenham(fb, x0, y0 - hdr, x0 + 7 * cw, y0 - hdr, pal::brown);

    // Days
    for (int d = 1; d <= 31; d++) {
        int col = (startCol + d - 1) % 7, row = (startCol + d - 1) / 7;
        int cx = x0 + col * cw, cy = y0 + row * ch;
        Vec2 mid{cx + cw / 2.0, cy + ch / 2.0};
        std::string num = std::to_string(d);

        if (d == today) {                       // highlighted: star with the number inside
            fillPolygon(fb, placed(makeStar({0,0}, 33, 15), 0, 1.0, {mid.x, mid.y + 2}), pal::mustard);
            drawText(fb, (int)mid.x - textW(num, 3) / 2, (int)mid.y - 8, num, 3, pal::dark);
            continue;
        }
        drawText(fb, cx + 8, cy + 8, num, 3, pal::dark);

        // Little stickers on days that "have a page saved"
        Vec2 corner{cx + cw - 22.0, cy + ch - 20.0};
        if (d == 1 || d == 4)  fillPolygon(fb, placed(makeHeart({0,0}, 22), 0, 1.0, corner), pal::terracotta);
        if (d == 2)            fillCircle(fb, (int)corner.x, (int)corner.y, 9, pal::green);
        if (d == 3)            fillPolygon(fb, placed(makeStar({0,0}, 12, 5), 0, 1.0, corner), pal::mustard);
    }
}

// ---------- entry point ----------
int main() {
    std::error_code error;
    std::filesystem::create_directories("output", error);
    if (error) {
        std::cerr << "Cannot create output directory: " << error.message() << "\n";
        return 1;
    }
    Framebuffer day(800, 600, pal::brown);
    buildDayPage(day);
    if (!day.saveBMP("output/day_page.bmp") || !day.savePPM("output/day_page.ppm")) {
        std::cerr << "Cannot save day page\n";
        return 1;
    }

    Framebuffer cal(800, 600, pal::brown);
    buildCalendar(cal);
    if (!cal.saveBMP("output/calendar.bmp") || !cal.savePPM("output/calendar.ppm")) {
        std::cerr << "Cannot save calendar\n";
        return 1;
    }

    std::cout << "Wrote output/day_page.* and output/calendar.*\n";
    return 0;
}