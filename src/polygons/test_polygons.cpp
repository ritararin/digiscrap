#include "polygons/polygons.h"
#include <iostream>
#include <cmath>

static int failures = 0;
#define CHECK(cond, name) do { \
    if (cond) std::cout << "PASS: " << name << "\n"; \
    else { std::cout << "FAIL: " << name << "\n"; failures++; } } while (0)

static bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }

static Polygon rotateAbout(const Polygon& p, Vec2 pv, double rad) {
    Polygon out;
    for (const Vec2& v : p) {
        double dx = v.x - pv.x, dy = v.y - pv.y;
        out.push_back({pv.x + dx*std::cos(rad) - dy*std::sin(rad),
                       pv.y + dx*std::sin(rad) + dy*std::cos(rad)});
    }
    return out;
}

static long spanPixels(const Polygon& p) {
    long n = 0;
    for (const Span& s : polygonSpans(p)) n += s.x1 - s.x0;
    return n;
}

int main() {
    // ---- Numeric checks ----
    Polygon sq = makeSquare(Vec2{100, 100}, 50);
    CHECK(near(polygonArea(sq), 2500), "square area == 2500");
    Vec2 c = polygonCentroid(sq);
    CHECK(near(c.x, 100) && near(c.y, 100), "square centroid == (100,100)");
    Vec2 mn, mx; boundingBox(sq, mn, mx);
    CHECK(near(mn.x,75) && near(mn.y,75) && near(mx.x,125) && near(mx.y,125), "square bounding box");

    CHECK(pointInPolygon(sq, Vec2{100,100}),  "square: centre inside");
    CHECK(!pointInPolygon(sq, Vec2{200,200}), "square: far point outside");
    CHECK(!pointInPolygon(sq, Vec2{74,100}),  "square: just-left point outside");

    Polygon star = makeStar(Vec2{300,300}, 80, 35);
    CHECK(pointInPolygon(star, Vec2{300,300}),  "star: centre inside");
    CHECK(!pointInPolygon(star, Vec2{600,600}), "star: far point outside");
    CHECK(!pointInPolygon(star, Vec2{300 + 76*std::cos(-M_PI/2 + M_PI/5),
                                     300 + 76*std::sin(-M_PI/2 + M_PI/5)}),
          "star: point in the notch between arms is outside");

    Polygon hex = makeRegularPolygon(Vec2{0,0}, 10, 6);
    CHECK(hex.size() == 6, "hexagon has 6 vertices");
    CHECK(near(polygonArea(hex), 1.5*std::sqrt(3.0)*100), "hexagon area matches formula");

    // Fill correctness: filled pixel count should be close to the true area
    CHECK(std::labs(spanPixels(sq) - 2500) <= 100, "fill: square pixel count ~ 2500");
    CHECK(std::fabs(spanPixels(star) - polygonArea(star)) < 0.03 * polygonArea(star),
          "fill: star pixel count within 3% of area");
    Polygon heart = makeHeart(Vec2{0,0}, 140);
    CHECK(std::fabs(spanPixels(heart) - polygonArea(heart)) < 0.03 * polygonArea(heart),
          "fill: heart pixel count within 3% of area");

    // ---- Visual test ----
    Framebuffer fb(800, 600);                    // ADAPT: constructor
    fb.clear(Color{250, 245, 235});              // ADAPT: clear + Color constructor

    fillPolygon(fb, makeStar(Vec2{120,120}, 80, 35),           Color{240,160,40});
    fillPolygon(fb, makeSquare(Vec2{300,120}, 100),            Color{120,170,230});
    fillPolygon(fb, makeRegularPolygon(Vec2{480,120}, 60, 6),  Color{110,150,120});
    fillPolygon(fb, makeHeart(Vec2{640,120}, 140),             Color{230,100,140});
    fillPolygon(fb, makeDateStamp(Vec2{140,300}, 200, 80),     Color{60,60,60});

    Polygon s2 = makeStar(Vec2{400,330}, 90, 40);
    fillPolygon(fb, rotateAbout(s2, polygonCentroid(s2), 30*M_PI/180), Color{180,90,60});

    fillPolygon(fb, makeSquare(Vec2{620,330}, 120), Color{120,170,230});
    fillPolygon(fb, rotateAbout(makeSquare(Vec2{650,360}, 120), Vec2{650,360}, 0.5), Color{240,160,40});

    fillPolygon(fb, makeStar(Vec2{780,580}, 100, 45), Color{90,90,90});   // runs off the page

    fb.saveBMP("output/polygons_test.bmp");      // ADAPT: save function
    std::cout << (failures == 0 ? "\nALL CHECKS PASSED\n" : "\nSOME CHECKS FAILED\n");
    return failures;
}