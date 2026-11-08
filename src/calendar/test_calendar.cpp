#include "calendar/calendar.h"
#include "animation/exporter.h"
#include "animation/timeline.h"
#include "polygons/polygons.h"
#include <cmath>
#include <iostream>

static int failures = 0;
#define CHECK(cond, name) do { \
    if (cond) std::cout << "PASS: " << name << "\n"; \
    else { std::cout << "FAIL: " << name << "\n"; failures++; } } while (0)

static bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }
static bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

int main() {
    CalendarLayout L;

    // ---------- numeric checks ----------
    Vec2 c1 = cellCentre(L, 1);
    CHECK(near(c1.x, 493) && near(c1.y, 194), "day 1 centre == (493,194)");

    bool allOk = true;
    for (int d = 1; d <= L.daysInMonth; d++) {
        Vec2 c = cellCentre(L, d);
        if (dayAt(L, (int)c.x, (int)c.y) != d) allOk = false;
    }
    CHECK(allOk, "dayAt(cellCentre(d)) == d for all 31 days");
    CHECK(dayAt(L, 10, 10) == 0, "point outside the grid has no day");
    CHECK(dayAt(L, L.x0 + 10, L.y0 + 10) == 0, "empty cell before day 1 has no day");

    int from = 5, to = 19;
    Vec2 s0 = highlightCentre(L, from, to, 0.0), s1 = highlightCentre(L, from, to, 1.0);
    Vec2 a = cellCentre(L, from), b = cellCentre(L, to);
    CHECK(near(s0.x, a.x) && near(s0.y, a.y), "highlight starts exactly in the source cell");
    CHECK(near(s1.x, b.x) && near(s1.y, b.y), "highlight ends exactly in the target cell");
    CHECK(dayAt(L, (int)s1.x, (int)s1.y) == to, "last frame: dayAt(highlight) == target day");

    bool onScreen = true;
    for (int i = 0; i <= 100; i++) {
        Vec2 p = highlightCentre(L, from, to, i / 100.0);
        if (p.x < 0 || p.x > kFrameW || p.y < 0 || p.y > kFrameH) onScreen = false;
    }
    CHECK(onScreen, "highlight path stays on screen");

    CHECK(near(smoothstep01(0), 0) && near(smoothstep01(1), 1), "easing: f(0)=0 and f(1)=1");
    CHECK(near(pulseRadius(20, 0.0, 3), 20) && near(pulseRadius(20, 1.0, 3), 20),
          "pulse starts and ends at base radius (loops seamlessly)");
    CHECK(pulseRadius(20, 0.25 / 3, 3) > 20, "pulse grows after t = 0");

    // ---------- pixel checks ----------
    Framebuffer fb(kFrameW, kFrameH, {0, 0, 0});
    drawCalendarBackdrop(fb, L);
    int px = L.x0 + 3 * L.cw, py = L.y0 + L.ch + 10;           // a spot on a vertical grid line
    drawGridProgress(fb, L, 0.0);
    CHECK(!same(pixelAt(fb, px, py), {154, 106, 67}), "t=0: grid line not drawn yet");
    drawGridProgress(fb, L, 1.0);
    CHECK(same(pixelAt(fb, px, py), {154, 106, 67}), "t=1: grid line fully drawn");

    Framebuffer z(kFrameW, kFrameH, {0, 0, 0}), cal(kFrameW, kFrameH, {0, 0, 0}),
                day(kFrameW, kFrameH, {250, 244, 230});
    drawCalendarFull(cal, L);
    fillPolygon(day, makeStar({400, 260}, 120, 52), {228, 165, 69});
    drawZoomTransition(z, cal, day, L, 5, 1.0);
    bool identical = true;
    for (int y = 0; y < kFrameH && identical; y++)
        for (int x = 0; x < kFrameW; x++)
            if (!same(pixelAt(z, x, y), pixelAt(day, x, y))) { identical = false; break; }
    CHECK(identical, "zoom at t=1 equals the day page exactly");
    drawZoomTransition(z, cal, day, L, 5, 0.0);
    CHECK(same(pixelAt(z, 10, 10), pixelAt(cal, 10, 10)), "zoom at t=0 still shows the calendar");

    // ---------- frame sequences through Ian's exporter ----------
    struct Seq { const char* name; RenderFrameFn fn; int frames; };
    Seq seqs[] = {
        {"calendar_grid",   renderCalendarGrid,   60},
        {"calendar_select", renderCalendarSelect, 60},
        {"calendar_pulse",  renderCalendarPulse,  60},
        {"calendar_zoom",   renderCalendarZoom,   40},
    };
    for (const Seq& s : seqs) {
        Timeline tl; tl.frames = s.frames;
        ExportResult r = exportAnimation(s.name, tl, kFrameW, kFrameH, Color{154, 106, 67}, s.fn);
        CHECK(r.ok && r.framesWritten == s.frames, std::string(s.name) + ": frames and contact sheet written");
    }

    std::cout << (failures == 0 ? "\nALL CHECKS PASSED\n" : "\nSOME CHECKS FAILED\n");
    return failures;
}