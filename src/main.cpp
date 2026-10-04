#include <iostream>
#include "core/framebuffer.h"
#include "core/mat3.h"
#include "core/scene.h"
#include "core/font.h"
#include "lines/lines.h"
#include "polygons/polygons.h"
#include "clipping/clipping.h"
#include "circles/circles.h"
#include "curves/curves.h"

static Vec2 centroid(const Polygon& p) {
    Vec2 c{0,0}; for (auto& v : p) { c.x += v.x; c.y += v.y; }
    return {c.x / p.size(), c.y / p.size()};
}
static Polygon placed(const Polygon& p, double deg, double s, Vec2 move) {
    return transformPolygon(p, Mat3::about(centroid(p), deg * M_PI / 180.0, s, move));
}

void buildDayPage(Framebuffer& fb) {
    Scene s;
    // 1. photo (procedural gradient until a PPM loader exists) + border
    s.add("photo", [](Framebuffer& f) {
        for (int y = 80; y < 380; y++) for (int x = 80; x < 460; x++)
            f.setPixel(x, y, {(uint8_t)(100 + (x-80)/3), (uint8_t)(150 + (y-80)/6), 200});
        Color k{60,40,30};
        drawLineBresenham(f, 80,80, 460,80, k);  drawLineBresenham(f, 460,80, 460,380, k);
        drawLineBresenham(f, 460,380, 80,380, k); drawLineBresenham(f, 80,380, 80,80, k);
    });
    // 2. stickers: transformed + filled
    s.add("star", [](Framebuffer& f) {
        fillPolygon(f, placed(makeStar({0,0}, 50, 22), 15, 1.2, {420, 140}), {255,200,0}); });
    s.add("heart", [](Framebuffer& f) {
        fillPolygon(f, placed(makeHeart({0,0}, 90), -20, 1.0, {200, 360}), {230,50,90}); });
    s.add("square", [](Framebuffer& f) {
        fillPolygon(f, placed(makeSquare({0,0}, 70), 30, 1.0, {560, 300}), {80,180,120}); });
    s.add("circle", [](Framebuffer& f) { fillCircle(f, 620, 150, 45, {120,100,220}); });
    // 3. clipped sticker: star cut by a window
    s.add("clipped", [](Framebuffer& f) {
        Polygon big = placed(makeStar({0,0}, 80, 35), 0, 1.0, {600, 480});
        fillPolygon(f, clipPolygon(big, 540, 440, 700, 520), {255,120,40}); });
    // 4. date stamp on top
    s.add("date", [](Framebuffer& f) { drawText(f, 90, 400, "04-10-2026", 4, {40,40,40}); });
    s.render(fb);
}

void buildCalendar(Framebuffer& fb) {
    int x0 = 50, y0 = 80, cw = 100, ch = 80;
    Color grid{90,90,90};
    for (int i = 0; i <= 7; i++) drawLineBresenham(fb, x0+i*cw, y0, x0+i*cw, y0+5*ch, grid);
    for (int j = 0; j <= 5; j++) drawLineBresenham(fb, x0, y0+j*ch, x0+7*cw, y0+j*ch, grid);
    int start = 4;                               // column of day 1 (demo: October 2026 starts Thursday)
    for (int d = 1; d <= 31; d++) {
        int col = (start + d - 1) % 7, row = (start + d - 1) / 7;
        int cx = x0 + col*cw, cy = y0 + row*ch;
        if (d == 4) fillPolygon(fb, makeSquare({cx + cw/2.0, cy + ch/2.0}, 70), {255,220,100}); // highlight
        drawText(fb, cx + 10, cy + 10, std::to_string(d), 4, {30,30,30});
    }
}

int main() {
    Framebuffer day(800, 600, {250,244,230});
    buildDayPage(day);
    day.saveBMP("output/day_page.bmp"); day.savePPM("output/day_page.ppm");

    Framebuffer cal(800, 600, {255,255,255});
    buildCalendar(cal);
    cal.saveBMP("output/calendar.bmp"); cal.savePPM("output/calendar.ppm");

    std::cout << "Wrote output/day_page.* and output/calendar.*\n";
}