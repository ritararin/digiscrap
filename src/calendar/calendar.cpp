#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "calendar/calendar.h"
#include "animation/timeline.h"      // segmentT
#include "polygons/polygons.h"
#include "lines/lines.h"
#include "circles/circles.h"
#include "core/font.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
const Color kBrown{154,106,67}, kDark{77,74,63}, kCream{249,228,192}, kPaper{252,241,217},
            kWeekend{247,222,178}, kGreen{110,145,121}, kMustard{228,165,69}, kShadow{214,186,140},
            kHoney{249,205,125}, kTerracotta{196,92,70}, kSage{173,170,134}, kWhite{255,252,245},
            kStitch{214,170,110}, kInk{30,22,15};

struct Seg { int ax, ay, bx, by; };
const Framebuffer* g_dayPage = nullptr;

int iround(double v) { return (int)std::lround(v); }
int atLeastOne(int r) { return r < 1 ? 1 : r; }

// ---------- easing and pseudo-random helpers ----------
double easeOutCubic(double t) { t = clamp01(t); double x = 1.0 - t; return 1.0 - x * x * x; }
double easeOutBack(double t) {                      // overshoots past 1, then settles back to exactly 1
    t = clamp01(t);
    const double c1 = 1.70158, c3 = c1 + 1.0, x = t - 1.0;
    return 1.0 + c3 * x * x * x + c1 * x * x;
}
double hash01(int n) {                              // deterministic "random" in [0,1)
    unsigned x = (unsigned)n * 2654435761u;
    x ^= x >> 15; x *= 2246822519u; x ^= x >> 13;
    return (x & 0xFFFFFFu) / 16777216.0;
}

// ---------- geometry helpers ----------
Polygon rectPoly(double x0, double y0, double x1, double y1) {
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
}
void fillRect(Framebuffer& f, double x0, double y0, double x1, double y1, Color c) {
    fillPolygon(f, rectPoly(x0, y0, x1, y1), c);
}
void drawOutline(Framebuffer& f, const Polygon& p, Color c) {
    for (size_t i = 0; i < p.size(); i++) {
        Vec2 a = p[i], b = p[(i + 1) % p.size()];
        drawLineBresenham(f, iround(a.x), iround(a.y), iround(b.x), iround(b.y), c);
    }
}
void drawRectLines(Framebuffer& f, int x0, int y0, int x1, int y1, Color c) {
    drawLineBresenham(f, x0, y0, x1, y0, c);
    drawLineBresenham(f, x1, y0, x1, y1, c);
    drawLineBresenham(f, x1, y1, x0, y1, c);
    drawLineBresenham(f, x0, y1, x0, y0, c);
}
Polygon placed(const Polygon& p, double deg, double s, Vec2 move) {
    return transformPolygon(p, Mat3::about(polygonCentroid(p), deg * M_PI / 180.0, s, move));
}

// ---------- alpha blending (source-over, written by hand) ----------
void blendPx(Framebuffer& f, int x, int y, Color c, double a) {
    if (a <= 0.0 || x < 0 || y < 0 || x >= f.width || y >= f.height) return;
    f.setPixel(x, y, lerp(pixelAt(f, x, y), c, clamp01(a)));
}
void fillPolygonAlpha(Framebuffer& f, const Polygon& p, Color c, double a) {
    if (a <= 0.0) return;
    for (const Span& s : polygonSpans(p))               // scanline fill, blended
        for (int x = s.x0; x < s.x1; x++) blendPx(f, x, s.y, c, a);
}
void fillDiscAlpha(Framebuffer& f, int cx, int cy, int r, Color c, double a) {
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r) blendPx(f, cx + x, cy + y, c, a);
}
// Soft glowing ring: strongest on the circle of radius r, fading over `thick` pixels
void softRing(Framebuffer& f, double cx, double cy, double r, double thick, Color c, double a) {
    if (a <= 0.0 || r <= 0.0) return;
    int x0 = (int)std::floor(cx - r - thick - 2), x1 = (int)std::ceil(cx + r + thick + 2);
    int y0 = (int)std::floor(cy - r - thick - 2), y1 = (int)std::ceil(cy + r + thick + 2);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            double e = std::fabs(std::hypot(x - cx, y - cy) - r);
            if (e < thick) blendPx(f, x, y, c, a * (1.0 - e / thick));
        }
}

// ---------- dashed stitching from Bresenham segments ----------
void dashedLine(Framebuffer& f, int x0, int y0, int x1, int y1, int dash, int gap, Color c) {
    double len = std::hypot((double)(x1 - x0), (double)(y1 - y0));
    if (len < 1.0) return;
    double dx = (x1 - x0) / len, dy = (y1 - y0) / len;
    for (double s = 0; s < len; s += dash + gap) {
        double e = std::min(s + dash, len);
        drawLineBresenham(f, iround(x0 + dx * s), iround(y0 + dy * s),
                          iround(x0 + dx * e), iround(y0 + dy * e), c);
    }
}
void dashedRect(Framebuffer& f, int x0, int y0, int x1, int y1, int dash, int gap, Color c) {
    dashedLine(f, x0, y0, x1, y0, dash, gap, c);
    dashedLine(f, x1, y0, x1, y1, dash, gap, c);
    dashedLine(f, x1, y1, x0, y1, dash, gap, c);
    dashedLine(f, x0, y1, x0, y0, dash, gap, c);
}

// ---------- scrapbook decoration ----------
void drawCard(Framebuffer& fb) {
    fb.clear(kBrown);
    for (int y = 60; y < 580; y += 70) fillCircle(fb, 28, y, 11, kDark);   // binder holes
    fillPolygon(fb, rectPoly(61, 37, 776, 577), kShadow);                  // drop shadow
    fillPolygon(fb, rectPoly(55, 30, 770, 570), kCream);                   // paper card
}
void drawTitle(Framebuffer& fb, const std::string& title, int visibleChars) {
    int n = std::max(0, std::min(visibleChars, (int)title.size()));
    if (n == 0) return;
    int x = 412 - textWidth(title, 5) / 2;
    std::string s = title.substr(0, n);
    drawText(fb, x + 3, 73, s, 5, kShadow);
    drawText(fb, x, 70, s, 5, kDark);
}
void drawDecor(Framebuffer& fb) {
    for (int i = 0; i < 700; i++) {                                        // paper speckle
        int x = 58 + (int)(hash01(i * 2 + 1) * 708), y = 34 + (int)(hash01(i * 2 + 2) * 532);
        blendPx(fb, x, y, kDark, 0.10 + 0.08 * hash01(i * 5 + 3));
    }
    dashedRect(fb, 63, 38, 762, 562, 9, 6, kStitch);                       // stitched border
}
void drawTape(Framebuffer& fb, double a) {
    if (a <= 0.0) return;
    Polygon tape = rectPoly(-52, -13, 52, 13);
    fillPolygonAlpha(fb, placed(tape, -24, 1.0, {100, 40}), kMustard, 0.78 * a);
    fillPolygonAlpha(fb, placed(tape, 20, 1.0, {735, 40}), kGreen, 0.78 * a);
}
void drawMiniStickers(Framebuffer& fb, const CalendarLayout& L, double pop) {
    if (pop <= 0.02) return;
    struct S { int day, kind; };
    static const S list[] = {{1,0},{2,1},{3,2},{8,3},{9,0},{15,2},{22,1},{26,0}};
    for (const S& s : list) {
        int x0, y0, x1, y1;
        cellRect(L, s.day, x0, y0, x1, y1);
        Vec2 p{x1 - 24.0, y1 - 22.0};
        switch (s.kind) {
            case 0: fillPolygon(fb, placed(makeHeart({0, 0}, 26), -12, pop, p), kTerracotta); break;
            case 1: fillCircle(fb, iround(p.x), iround(p.y), atLeastOne(iround(9 * pop)), kGreen); break;
            case 2: fillPolygon(fb, placed(makeStar({0, 0}, 13, 6), 10, pop, p), kMustard); break;
            default: fillPolygon(fb, placed(makeRegularPolygon({0, 0}, 11, 6), 0, pop, p), kSage); break;
        }
    }
}
void drawNumbersExcept(Framebuffer& fb, const CalendarLayout& L, int skipA, int skipB) {
    for (int d = 1; d <= L.daysInMonth; d++) {
        if (d == skipA || d == skipB) continue;
        int x0, y0, x1, y1;
        cellRect(L, d, x0, y0, x1, y1);
        drawText(fb, x0 + 8, y0 + 8, std::to_string(d), 3, kDark);
    }
}

// Today's star: soft ground shadow, outline, lighter inner star for a bevel, number inside.
// `lift` (0..1) pushes the shadow away so the star looks raised off the page.
void drawStarMarker(Framebuffer& fb, Vec2 c, double scale, double deg, double lift, int day) {
    Polygon star = placed(makeStar({0, 0}, 33, 15), deg, scale, c);
    Polygon sh = star;
    double dx = 4 + 22 * lift, dy = 5 + 30 * lift;
    for (Vec2& v : sh) { v.x += dx; v.y += dy; }
    fillPolygonAlpha(fb, sh, kDark, 0.28);
    fillPolygon(fb, star, kMustard);
    drawOutline(fb, star, {150, 100, 30});
    fillPolygon(fb, placed(makeStar({0, 0}, 22, 10), deg, scale, c), kHoney);
    std::string num = std::to_string(day);
    drawText(fb, iround(c.x) - textWidth(num, 3) / 2, iround(c.y) - 8, num, 3, kDark);
}

// Confetti burst: stars, squares and discs fly out, spin, fall and fade (t from 0 to 1)
void drawConfetti(Framebuffer& fb, Vec2 origin, double t, int count, int seed) {
    if (t <= 0.0 || t >= 1.0) return;
    const Color cols[5] = {kMustard, kTerracotta, kGreen, kHoney, kSage};
    for (int i = 0; i < count; i++) {
        double ang = 2.0 * M_PI * hash01(seed + i * 3);
        double dist = (70.0 + 110.0 * hash01(seed + i * 3 + 1)) * easeOutCubic(t);
        Vec2 p{origin.x + std::cos(ang) * dist, origin.y + std::sin(ang) * dist + 90.0 * t * t};
        double a = 1.0 - t * t;
        double spin = 720.0 * hash01(seed + i * 3 + 2) * t;
        Color c = cols[i % 5];
        switch (i % 3) {
            case 0: fillPolygonAlpha(fb, placed(makeStar({0, 0}, 8, 3.5), spin, 1.0, p), c, a); break;
            case 1: fillPolygonAlpha(fb, placed(makeSquare({0, 0}, 9), spin, 1.0, p), c, a); break;
            default: fillDiscAlpha(fb, iround(p.x), iround(p.y), 4, c, a); break;
        }
    }
}

// Complete decorated calendar (static). Numbers of skipA and skipB are left out
// because a star marker draws them.
void drawScenePretty(Framebuffer& fb, const CalendarLayout& L, int skipA, int skipB) {
    drawCalendarBackdrop(fb, L);
    drawDecor(fb);
    drawGridProgress(fb, L, 1.0);
    drawNumbersExcept(fb, L, skipA, skipB);
    drawMiniStickers(fb, L, 1.0);
    drawTape(fb, 1.0);
}

// Placeholder day page used by the zoom until the real one is plugged in
void makeDayPlaceholder(Framebuffer& fb) {
    fb.clear(kBrown);
    for (int y = 60; y < 580; y += 70) fillCircle(fb, 28, y, 11, kDark);
    fillPolygon(fb, rectPoly(76, 37, 776, 577), kShadow);
    fillPolygon(fb, rectPoly(70, 30, 770, 570), kCream);
    fillPolygon(fb, rectPoly(136, 86, 476, 426), kShadow);
    fillPolygon(fb, rectPoly(130, 80, 470, 420), kWhite);                     // polaroid
    for (int y = 100; y < 350; y++)                                           // sky gradient
        for (int x = 150; x < 450; x++)
            fb.setPixel(x, y, lerp(Color{150, 190, 200}, kHoney, (y - 100) / 250.0));
    fillCircle(fb, 380, 170, 28, {255, 236, 160});                            // sun
    fillPolygon(fb, {{150,350},{150,300},{230,260},{320,305},{400,275},{450,300},{450,350}},
                Color{84, 118, 96});                                          // hills
    drawRectLines(fb, 150, 100, 450, 350, kDark);
    fillPolygon(fb, placed(makeStar({0, 0}, 50, 22), 15, 1.2, {520, 150}), kMustard);
    fillPolygon(fb, placed(makeHeart({0, 0}, 90), -20, 1.0, {190, 330}), kTerracotta);
    Polygon stamp = makeDateStamp({620, 400}, 240, 60, 10);
    fillPolygon(fb, stamp, kWhite);
    drawOutline(fb, stamp, kBrown);
    drawText(fb, 620 - textWidth("05-10-2026", 4) / 2, 390, "05-10-2026", 4, kDark);
}
} // namespace

// ================= helpers =================
double smoothstep01(double t) { t = clamp01(t); return t * t * (3 - 2 * t); }
double pulseRadius(double base, double t, int pulses) {
    return base * (1.0 + 0.25 * std::sin(2.0 * M_PI * pulses * t));
}

void cellRect(const CalendarLayout& L, int day, int& x0, int& y0, int& x1, int& y1) {
    int idx = L.startCol + day - 1;
    int col = idx % 7, row = idx / 7;
    x0 = L.x0 + col * L.cw;  y0 = L.y0 + row * L.ch;
    x1 = x0 + L.cw;          y1 = y0 + L.ch;
}
Vec2 cellCentre(const CalendarLayout& L, int day) {
    int x0, y0, x1, y1;
    cellRect(L, day, x0, y0, x1, y1);
    return {(x0 + x1) / 2.0, (y0 + y1) / 2.0};
}
int dayAt(const CalendarLayout& L, int px, int py) {
    if (px < L.x0 || py < L.y0) return 0;
    int col = (px - L.x0) / L.cw, row = (py - L.y0) / L.ch;
    if (col >= 7 || row >= L.rows()) return 0;
    int day = row * 7 + col - L.startCol + 1;
    return (day >= 1 && day <= L.daysInMonth) ? day : 0;
}

// ================= static parts =================
void drawCalendarBackdrop(Framebuffer& fb, const CalendarLayout& L) {
    drawCard(fb);
    drawTitle(fb, "OCTOBER 2026", 12);

    const char* days[7] = {"S", "M", "T", "W", "T", "F", "S"};
    for (int c = 0; c < 7; c++) {
        fillRect(fb, L.x0 + c * L.cw, L.y0 - L.hdr, L.x0 + (c + 1) * L.cw, L.y0, kGreen);
        drawText(fb, L.x0 + c * L.cw + L.cw / 2 - 10, L.y0 - L.hdr + 7, days[c], 3, kPaper);
    }
    for (int r = 0; r < L.rows(); r++)
        for (int c = 0; c < 7; c++)
            fillRect(fb, L.x0 + c * L.cw, L.y0 + r * L.ch, L.x0 + (c + 1) * L.cw,
                     L.y0 + (r + 1) * L.ch, (c == 0 || c == 6) ? kWeekend : kPaper);
}

void drawDayNumbers(Framebuffer& fb, const CalendarLayout& L, int visible, int skipDay) {
    for (int d = 1; d <= std::min(visible, L.daysInMonth); d++) {
        if (d == skipDay) continue;
        int x0, y0, x1, y1;
        cellRect(L, d, x0, y0, x1, y1);
        drawText(fb, x0 + 8, y0 + 8, std::to_string(d), 3, kDark);
    }
}

// ================= 1. Grid draws itself =================
// Each cell's four edges are Bresenham lines whose far ends grow from the start.
// Cells are staggered so the grid appears cell by cell.
void drawGridProgress(Framebuffer& fb, const CalendarLayout& L, double t) {
    std::vector<Seg> segs;
    auto addCell = [&](int x0, int y0, int x1, int y1) {
        segs.push_back({x0, y0, x1, y0});
        segs.push_back({x1, y0, x1, y1});
        segs.push_back({x1, y1, x0, y1});
        segs.push_back({x0, y1, x0, y0});
    };
    int yt = L.y0 - L.hdr;
    for (int c = 0; c < 7; c++)
        addCell(L.x0 + c * L.cw, yt, L.x0 + (c + 1) * L.cw, L.y0);
    for (int r = 0; r < L.rows(); r++)
        for (int c = 0; c < 7; c++)
            addCell(L.x0 + c * L.cw, L.y0 + r * L.ch,
                    L.x0 + (c + 1) * L.cw, L.y0 + (r + 1) * L.ch);

    const double span = 0.45;
    const int n = (int)segs.size();
    if (n == 0) return;
    for (int k = 0; k < n; k++) {
        double start = (n == 1) ? 0.0 : (1.0 - span) * k / (n - 1);
        double p = clamp01((t - start) / span);
        if (p <= 0) continue;
        const Seg& s = segs[k];
        drawLineBresenham(fb, s.ax, s.ay,
                          iround(lerp((double)s.ax, (double)s.bx, p)),
                          iround(lerp((double)s.ay, (double)s.by, p)), kBrown);
    }
}

void drawCalendarFull(Framebuffer& fb, const CalendarLayout& L) {
    drawCalendarBackdrop(fb, L);
    drawGridProgress(fb, L, 1.0);
    drawDayNumbers(fb, L, L.daysInMonth);
}

// ================= 2. Highlight glides cell to cell =================
// de Casteljau evaluation of a quadratic Bezier arching above the two cells.
Vec2 highlightCentre(const CalendarLayout& L, int fromDay, int toDay, double t) {
    Vec2 A = cellCentre(L, fromDay), B = cellCentre(L, toDay);
    Vec2 C{(A.x + B.x) / 2.0, std::min(A.y, B.y) - 70.0};            // control point
    double u = smoothstep01(t);
    return lerp(lerp(A, C, u), lerp(C, B, u), u);
}

void drawHighlightMove(Framebuffer& fb, const CalendarLayout& L, int fromDay, int toDay, double t) {
    double u = smoothstep01(t);
    drawScenePretty(fb, L, u > 0.2 ? 0 : fromDay, toDay);

    // destination: honey tint and a ring that closes in on the cell
    int x0, y0, x1, y1;
    cellRect(L, toDay, x0, y0, x1, y1);
    fillPolygonAlpha(fb, rectPoly(x0 + 3, y0 + 3, x1 - 3, y1 - 3), kHoney, 0.7 * u);
    Vec2 tc = cellCentre(L, toDay);
    softRing(fb, tc.x, tc.y, lerp(70.0, 34.0, u), 3.0, kTerracotta,
             0.55 * u * (1.0 - segmentT(t, 0.9, 1.0)));

    // motion trail: ghost stars sampled from earlier on the same Bezier path
    double fade = 1.0 - segmentT(t, 0.82, 0.98);
    for (int k = 6; k >= 1; k--) {
        double tt = t - k * 0.03;
        if (tt <= 0.0) continue;
        Vec2 p = highlightCentre(L, fromDay, toDay, tt);
        double a = 0.38 * (1.0 - k / 7.0) * fade;
        fillPolygonAlpha(fb, placed(makeStar({0, 0}, 33, 15), 360.0 * smoothstep01(tt),
                                    0.55 * (1.0 - k * 0.06), p), kMustard, a);
    }

    // the star: rotates one turn, swells mid-flight, small bounce on landing
    double land = segmentT(t, 0.82, 1.0);
    double scale = (1.0 + 0.35 * std::sin(M_PI * u)) * (1.0 - 0.14 * std::sin(M_PI * land));
    Vec2 c = highlightCentre(L, fromDay, toDay, t);
    drawStarMarker(fb, c, scale, 360.0 * u, std::sin(M_PI * u), toDay);

    // landing burst: expanding ring plus confetti
    double bt = segmentT(t, 0.86, 1.0);
    softRing(fb, tc.x, tc.y, lerp(30.0, 95.0, bt), 4.0, kMustard, 0.55 * (1.0 - bt));
    drawConfetti(fb, tc, bt, 18, 11);
}

// ================= 3. Pulsing circle and ellipse =================
void drawPulsingCircle(Framebuffer& fb, Vec2 c, double baseR, double t, int pulses,
                       Color fill, Color edge) {
    int r = atLeastOne(iround(pulseRadius(baseR, t, pulses)));
    fillCircle(fb, iround(c.x), iround(c.y), r, fill);
    drawCircleMidpoint(fb, iround(c.x), iround(c.y), r, edge);
}

void drawPulsingEllipse(Framebuffer& fb, Vec2 c, double baseRx, double baseRy, double t,
                        int pulses, Color fill, Color edge) {
    double k = std::sin(2.0 * M_PI * pulses * t);
    int rx = atLeastOne(iround(baseRx * (1.0 + 0.25 * k)));
    int ry = atLeastOne(iround(baseRy * (1.0 - 0.25 * k)));
    fillEllipse(fb, iround(c.x), iround(c.y), rx, ry, fill);
    drawEllipseMidpoint(fb, iround(c.x), iround(c.y), rx, ry, edge);
}

// ================= 4. Cell zooms into the day page =================
// The calendar dims, a cream photo frame with a drop shadow grows from the cell
// to the full screen, and each pixel inside samples the day page by inverse mapping.
void drawZoomTransition(Framebuffer& dst, const Framebuffer& calendarFrame,
                        const Framebuffer& dayFrame, const CalendarLayout& L, int day, double t) {
    int cx0, cy0, cx1, cy1;
    cellRect(L, day, cx0, cy0, cx1, cy1);
    double u = smoothstep01(t);
    int rx0 = iround(lerp((double)cx0, 0.0, u)),       ry0 = iround(lerp((double)cy0, 0.0, u));
    int rx1 = iround(lerp((double)cx1, (double)kFrameW, u)),
        ry1 = iround(lerp((double)cy1, (double)kFrameH, u));

    double dim = 0.55 * u;                                           // 0 at t=0, so t=0 is the pure calendar
    for (int y = 0; y < kFrameH; y++)
        for (int x = 0; x < kFrameW; x++) {
            Color p = pixelAt(calendarFrame, x, y);
            if (dim > 0.0) p = lerp(p, kInk, dim);
            dst.setPixel(x, y, p);
        }

    if (t < 1.0) {
        fillPolygonAlpha(dst, rectPoly(rx0 + 10, ry0 + 12, rx1 + 10, ry1 + 12), kInk, 0.38);
        fillRect(dst, rx0 - 6, ry0 - 6, rx1 + 6, ry1 + 6, kCream);   // photo frame
    }

    int w = rx1 - rx0, h = ry1 - ry0;
    if (w > 0 && h > 0) {
        for (int y = std::max(ry0, 0); y < std::min(ry1, kFrameH); y++)
            for (int x = std::max(rx0, 0); x < std::min(rx1, kFrameW); x++) {
                int sx = (int)((long)(x - rx0) * dayFrame.width / w);
                int sy = (int)((long)(y - ry0) * dayFrame.height / h);
                dst.setPixel(x, y, pixelAt(dayFrame, sx, sy));
            }
    }
    if (t < 1.0) drawRectLines(dst, rx0, ry0, rx1 - 1, ry1 - 1, kDark);
}

// ================= frame functions for the animation exporter =================
void renderCalendarGrid(Framebuffer& fb, double t) {
    CalendarLayout L;
    drawCard(fb);
    drawDecor(fb);

    const std::string title = "OCTOBER 2026";                       // types itself out
    drawTitle(fb, title, (int)std::ceil(segmentT(t, 0.04, 0.30) * title.size()));

    const char* days[7] = {"S", "M", "T", "W", "T", "F", "S"};      // headers drop in
    for (int c = 0; c < 7; c++) {
        double s = segmentT(t, 0.10 + 0.02 * c, 0.32 + 0.02 * c);
        if (s <= 0) continue;
        int off = iround(-(1.0 - easeOutBack(s)) * 24.0);
        int hx = L.x0 + c * L.cw, hy = L.y0 - L.hdr + off;
        fillPolygonAlpha(fb, rectPoly(hx, hy, hx + L.cw, hy + L.hdr), kGreen, clamp01(s * 1.6));
        if (s > 0.5) drawText(fb, hx + L.cw / 2 - 10, hy + 7, days[c], 3, kPaper);
    }

    for (int r = 0; r < L.rows(); r++)                              // cells pop in on a diagonal wave
        for (int c = 0; c < 7; c++) {
            double delay = 0.025 * (r + c);
            double p = easeOutBack(segmentT(t, 0.18 + delay, 0.43 + delay));
            if (p <= 0.02) continue;
            Vec2 ctr{L.x0 + c * L.cw + L.cw / 2.0, L.y0 + r * L.ch + L.ch / 2.0};
            Polygon cell = placed(rectPoly(-L.cw / 2.0, -L.ch / 2.0, L.cw / 2.0, L.ch / 2.0), 0, p, ctr);
            fillPolygon(fb, cell, (c == 0 || c == 6) ? kWeekend : kPaper);
        }

    drawGridProgress(fb, L, segmentT(t, 0.42, 0.88));               // Bresenham lines draw over them
    double fin = segmentT(t, 0.86, 1.0);
    drawDayNumbers(fb, L, (int)std::ceil(segmentT(t, 0.5, 0.88) * L.daysInMonth), fin > 0 ? 5 : 0);
    drawMiniStickers(fb, L, easeOutBack(segmentT(t, 0.78, 0.93)));
    drawTape(fb, segmentT(t, 0.0, 0.15));

    if (fin > 0) {                                                  // today's star stamps in
        Vec2 c5 = cellCentre(L, 5);
        drawStarMarker(fb, c5, easeOutBack(fin), 180.0 * (1.0 - easeOutCubic(fin)),
                       0.7 * std::sin(M_PI * fin), 5);
        drawConfetti(fb, c5, segmentT(t, 0.88, 1.0), 22, 7);
    }
}

void renderCalendarSelect(Framebuffer& fb, double t) {
    CalendarLayout L;
    drawHighlightMove(fb, L, 5, 19, t);
}

void renderCalendarPulse(Framebuffer& fb, double t) {
    CalendarLayout L;
    drawScenePretty(fb, L, 5, 0);
    Vec2 c5 = cellCentre(L, 5);

    for (int k = 0; k < 3; k++) {                                   // ripples radiating from today
        double ph = std::fmod(t * 2.0 + k / 3.0, 1.0);
        softRing(fb, c5.x, c5.y, lerp(30.0, 95.0, ph), 3.5, kMustard, 0.6 * (1.0 - ph));
    }
    drawStarMarker(fb, c5, 1.0 + 0.06 * std::sin(2.0 * M_PI * 2.0 * t), 0.0, 0.0, 5);

    int x0, y0, x1, y1;
    cellRect(L, 5, x0, y0, x1, y1);                                 // midpoint-circle notification badge
    drawPulsingCircle(fb, {x1 - 14.0, y0 + 14.0}, 8, t, 2, kTerracotta, {120, 50, 35});

    Vec2 c12 = cellCentre(L, 12);                                   // midpoint-ellipse badge with bobbing heart
    drawPulsingEllipse(fb, c12, 26, 16, t, 2, kGreen, {70, 100, 80});
    Vec2 hp{c12.x, c12.y - 1.0 + 3.0 * std::sin(2.0 * M_PI * 2.0 * t)};
    fillPolygon(fb, placed(makeHeart({0, 0}, 18), 0, 1.0, hp), kPaper);
}

void setZoomDayPage(const Framebuffer* dayPage) { g_dayPage = dayPage; }

void renderCalendarZoom(Framebuffer& fb, double t) {
    static Framebuffer cal(kFrameW, kFrameH, kBrown), placeholder(kFrameW, kFrameH, kBrown);
    static bool built = false;
    CalendarLayout L;
    if (!built) {
        drawScenePretty(cal, L, 5, 0);
        drawStarMarker(cal, cellCentre(L, 5), 1.0, 0.0, 0.0, 5);
        makeDayPlaceholder(placeholder);
        built = true;
    }
    drawZoomTransition(fb, cal, g_dayPage ? *g_dayPage : placeholder, L, 5, t);
}