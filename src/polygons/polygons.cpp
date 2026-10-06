#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "polygons/polygons.h"
#include <cmath>
#include <algorithm>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Polygon makeSquare(Vec2 c, double s) { double h = s/2; return {{c.x-h,c.y-h},{c.x+h,c.y-h},{c.x+h,c.y+h},{c.x-h,c.y+h}}; }
Polygon makeStar(Vec2 c, double R, double r) {
    Polygon p;
    for (int i = 0; i < 10; i++) {
        double a = -M_PI/2 + i * M_PI/5, d = (i % 2 == 0) ? R : r;
        p.push_back({c.x + d*std::cos(a), c.y + d*std::sin(a)});
    }
    return p;
}
Polygon makeHeart(Vec2 c, double size) {
    Polygon p;
    for (int i = 0; i < 40; i++) {
        double t = 2*M_PI*i/40;
        double x = 16*std::pow(std::sin(t),3);
        double y = -(13*std::cos(t) - 5*std::cos(2*t) - 2*std::cos(3*t) - std::cos(4*t));
        p.push_back({c.x + x*size/32, c.y + y*size/32});
    }
    return p;
}
// ===================== Extra generators =====================
Polygon makeRegularPolygon(Vec2 c, double R, int sides) {
    Polygon p;
    for (int i = 0; i < sides; i++) {
        double a = -M_PI/2 + i * 2*M_PI/sides;
        p.push_back({c.x + R*std::cos(a), c.y + R*std::sin(a)});
    }
    return p;
}

// Postage-stamp rectangle with zigzag notches on the top and bottom edges
Polygon makeDateStamp(Vec2 c, double w, double h, int notches) {
    Polygon p;
    double left = c.x - w/2, right = c.x + w/2;
    double top = c.y - h/2, bottom = c.y + h/2;
    double step = w / notches, depth = h * 0.08;
    for (int i = 0; i < notches; i++) {
        p.push_back({left + i*step,            top});
        p.push_back({left + i*step + step/2,   top + depth});
    }
    p.push_back({right, top});
    p.push_back({right, bottom});
    for (int i = notches - 1; i >= 0; i--) {
        p.push_back({left + i*step + step/2,   bottom - depth});
        p.push_back({left + i*step,            bottom});
    }
    return p;
}

// Classic ribbon banner with V-notches (swallowtail cutouts) at the left and right ends
Polygon makeRibbon(Vec2 c, double w, double h, double notchDepth) {
    if (notchDepth < 0.0) notchDepth = h * 0.4;
    double halfW = w / 2.0, halfH = h / 2.0;
    Polygon p;
    p.push_back({c.x - halfW, c.y - halfH});                  // Top-left
    p.push_back({c.x + halfW, c.y - halfH});                  // Top-right
    p.push_back({c.x + halfW - notchDepth, c.y});             // Right notch (inward)
    p.push_back({c.x + halfW, c.y + halfH});                  // Bottom-right
    p.push_back({c.x - halfW, c.y + halfH});                  // Bottom-left
    p.push_back({c.x - halfW + notchDepth, c.y});             // Left notch (inward)
    return p;
}

// Flowing ribbon banner with sinusoidal wave curvature and notched ends
Polygon makeWavyRibbon(Vec2 c, double w, double h, double waveAmp, double waveFreq, double notchDepth, int samples) {
    if (notchDepth < 0.0) notchDepth = h * 0.35;
    if (samples < 8) samples = 8;
    double halfW = w / 2.0, halfH = h / 2.0;
    Polygon p;
    // Top edge along the wave from left to right
    for (int i = 0; i <= samples; i++) {
        double u = (double)i / samples;
        double x = c.x - halfW + u * w;
        double y = c.y - halfH + waveAmp * std::sin(2.0 * M_PI * waveFreq * u);
        p.push_back({x, y});
    }
    // Right notch
    double rightWaveY = waveAmp * std::sin(2.0 * M_PI * waveFreq * 1.0);
    p.push_back({c.x + halfW - notchDepth, c.y + rightWaveY});
    p.push_back({c.x + halfW, c.y + halfH + rightWaveY});
    // Bottom edge along the wave from right to left
    for (int i = samples; i >= 0; i--) {
        double u = (double)i / samples;
        double x = c.x - halfW + u * w;
        double y = c.y + halfH + waveAmp * std::sin(2.0 * M_PI * waveFreq * u);
        p.push_back({x, y});
    }
    // Left notch
    double leftWaveY = waveAmp * std::sin(0.0);
    p.push_back({c.x - halfW + notchDepth, c.y + leftWaveY});
    return p;
}

// ===================== Operations =====================


void boundingBox(const Polygon& p, Vec2& minP, Vec2& maxP) {
    if (p.empty()) { minP = maxP = {0, 0}; return; }
    minP = maxP = p[0];
    for (const Vec2& v : p) {
        minP.x = std::min(minP.x, v.x);  minP.y = std::min(minP.y, v.y);
        maxP.x = std::max(maxP.x, v.x);  maxP.y = std::max(maxP.y, v.y);
    }
}

static double signedArea(const Polygon& p) {          // shoelace formula
    double s = 0;
    for (size_t i = 0; i < p.size(); i++) {
        const Vec2& a = p[i]; const Vec2& b = p[(i + 1) % p.size()];
        s += a.x*b.y - b.x*a.y;
    }
    return s / 2.0;
}

double polygonArea(const Polygon& p) { return std::fabs(signedArea(p)); }

Vec2 polygonCentroid(const Polygon& p) {
    double A = signedArea(p);
    if (std::fabs(A) < 1e-9) {                          // degenerate: average the vertices
        Vec2 s{0, 0};
        for (const Vec2& v : p) { s.x += v.x; s.y += v.y; }
        if (!p.empty()) { s.x /= p.size(); s.y /= p.size(); }
        return s;
    }
    double cx = 0, cy = 0;
    for (size_t i = 0; i < p.size(); i++) {
        const Vec2& a = p[i]; const Vec2& b = p[(i + 1) % p.size()];
        double cr = a.x*b.y - b.x*a.y;
        cx += (a.x + b.x) * cr;
        cy += (a.y + b.y) * cr;
    }
    return {cx / (6*A), cy / (6*A)};
}

// Even-odd ray casting
bool pointInPolygon(const Polygon& p, Vec2 pt) {
    if (p.size() < 3) return false;
    bool inside = false;
    for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++) {
        bool crosses = (p[i].y > pt.y) != (p[j].y > pt.y);
        if (crosses && pt.x < (p[j].x - p[i].x) * (pt.y - p[i].y)
                              / (p[j].y - p[i].y) + p[i].x)
            inside = !inside;
    }
    return inside;
}

// ===================== Scanline fill =====================
std::vector<Span> polygonSpans(const Polygon& poly) {
    std::vector<Span> spans;
    int n = (int)poly.size();
    if (n < 3) return spans;

    // Edge table: every non-horizontal edge, stored upper endpoint first
    struct Edge { double yMin, yMax, xAtYMin, invSlope; };
    std::vector<Edge> table;
    double top = poly[0].y, bottom = poly[0].y;
    for (int i = 0; i < n; i++) {
        Vec2 a = poly[i], b = poly[(i + 1) % n];
        top    = std::min(top,    std::min(a.y, b.y));
        bottom = std::max(bottom, std::max(a.y, b.y));
        if (a.y == b.y) continue;
        if (a.y > b.y) std::swap(a, b);
        table.push_back({a.y, b.y, a.x, (b.x - a.x) / (b.y - a.y)});
    }
    std::sort(table.begin(), table.end(),
              [](const Edge& e, const Edge& f) { return e.yMin < f.yMin; });

    // Walk down the rows, maintaining the active edge list
    std::vector<Edge> active;
    size_t next = 0;
    for (int y = (int)std::floor(top); y <= (int)std::ceil(bottom); y++) {
        double sy = y + 0.5;                              // sample at pixel centre
        while (next < table.size() && table[next].yMin <= sy)
            active.push_back(table[next++]);              // edges starting
        active.erase(std::remove_if(active.begin(), active.end(),
                     [&](const Edge& e) { return e.yMax <= sy; }),
                     active.end());                       // edges finished

        std::vector<double> xs;
        for (const Edge& e : active)
            xs.push_back(e.xAtYMin + (sy - e.yMin) * e.invSlope);
        std::sort(xs.begin(), xs.end());

        for (size_t k = 0; k + 1 < xs.size(); k += 2)     // even-odd: fill pairs
            spans.push_back({y, (int)std::ceil(xs[k] - 0.5),
                                (int)std::ceil(xs[k+1] - 0.5)});
    }
    return spans;
}

void fillPolygon(Framebuffer& fb, const Polygon& p, Color c) {
    for (const Span& s : polygonSpans(p))
        for (int x = s.x0; x < s.x1; x++)
            fb.setPixel(x, s.y, c);                       // ADAPT if setPixel differs
}