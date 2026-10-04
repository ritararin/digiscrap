#include <iostream>
#include <string>
#include "core/framebuffer.h"
#include "core/mat3.h"
#include "core/scene.h"
#include "core/font.h"
#include "lines/lines.h"
#include "polygons/polygons.h"
#include "clipping/clipping.h"
#include "circles/circles.h"
#include "curves/curves.h"

// helpers 

static Vec2 centroid(const Polygon& p) {
    Vec2 c{0, 0};
    for (auto& v : p) { c.x += v.x; c.y += v.y; }
    return {c.x / p.size(), c.y / p.size()};
}

// Rotate (degrees) and scale about the polygon's own centre, then move it
static Polygon placed(const Polygon& p, double deg, double s, Vec2 move) {
    return transformPolygon(p, Mat3::about(centroid(p), deg * M_PI / 180.0, s, move));
}

// Outline of a polygon using our line algorithm (proves the transforms work
// even while fillPolygon is still a placeholder)
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

//  day page 

void buildDayPage(Framebuffer& fb) {
    Scene s;

    // 1. Photo (procedural gradient until a PPM loader exists) + border
    s.add("photo", [](Framebuffer& f) {
        for (int y = 80; y < 380; y++)
            for (int x = 80; x < 460; x++)
                f.setPixel(x, y, {(uint8_t)(100 + (x - 80) / 3),
                                  (uint8_t)(150 + (y - 80) / 6), 200});
        drawRect(f, 80, 80, 460, 380, {60, 40, 30});
    });

    // 2. Stickers: transformed with Mat3, filled, then outlined
    s.add("star", [](Framebuffer& f) {
        Polygon p = placed(makeStar({0, 0}, 50, 22), 15, 1.2, {420, 140});
        fillPolygon(f, p, {255, 200, 0});
        drawOutline(f, p, {150, 110, 0});
    });

    s.add("heart", [](Framebuffer& f) {
        // moved inside the photo so it no longer overlaps the date stamp
        Polygon p = placed(makeHeart({0, 0}, 90), -20, 1.0, {170, 300});
        fillPolygon(f, p, {230, 50, 90});
        drawOutline(f, p, {120, 20, 50});
    });

    s.add("square", [](Framebuffer& f) {
        Polygon p = placed(makeSquare({0, 0}, 70), 30, 1.0, {560, 300});
        fillPolygon(f, p, {80, 180, 120});
        drawOutline(f, p, {20, 80, 50});
    });

    s.add("circle", [](Framebuffer& f) {
        fillCircle(f, 620, 150, 45, {120, 100, 220});
        drawCircleMidpoint(f, 620, 150, 45, {60, 40, 140});
    });

    // 3. Clipped sticker: star cut by a window (window outlined for clarity)
    s.add("clipped", [](Framebuffer& f) {
        Polygon big = placed(makeStar({0, 0}, 80, 35), 0, 1.0, {600, 480});
        Polygon cut = clipPolygon(big, 540, 440, 700, 520);
        fillPolygon(f, cut, {255, 120, 40});
        drawOutline(f, cut, {140, 60, 10});
        drawRect(f, 540, 440, 700, 520, {90, 90, 90});
    });

    // 4. Date stamp on top (placed below the photo, clear of the stickers)
    s.add("date", [](Framebuffer& f) {
        drawText(f, 90, 405, "04-10-2026", 4, {40, 40, 40});
    });

    s.render(fb);
}

//  calendar 

void buildCalendar(Framebuffer& fb) {
    int x0 = 50, y0 = 80, cw = 100, ch = 80;
    Color grid{90, 90, 90};

    for (int i = 0; i <= 7; i++)
        drawLineBresenham(fb, x0 + i * cw, y0, x0 + i * cw, y0 + 5 * ch, grid);
    for (int j = 0; j <= 5; j++)
        drawLineBresenham(fb, x0, y0 + j * ch, x0 + 7 * cw, y0 + j * ch, grid);

    int start = 4;   // October 2026 starts on a Thursday (Sunday = column 0)
    for (int d = 1; d <= 31; d++) {
        int col = (start + d - 1) % 7, row = (start + d - 1) / 7;
        int cx = x0 + col * cw, cy = y0 + row * ch;

        if (d == 4)  // highlighted day
            fillPolygon(fb, makeSquare({cx + cw / 2.0, cy + ch / 2.0}, 70), {255, 220, 100});

        drawText(fb, cx + 10, cy + 10, std::to_string(d), 4, {30, 30, 30});
    }
}

//  entry point 

int main() {
    Framebuffer day(800, 600, {250, 244, 230});
    buildDayPage(day);
    day.saveBMP("output/day_page.bmp");
    day.savePPM("output/day_page.ppm");

    Framebuffer cal(800, 600, {255, 255, 255});
    buildCalendar(cal);
    cal.saveBMP("output/calendar.bmp");
    cal.savePPM("output/calendar.ppm");

    std::cout << "Wrote output/day_page.* and output/calendar.*\n";
    return 0;
}