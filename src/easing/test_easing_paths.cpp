// Self-checking tests for easing and motion paths (no framework). Build/run: see Makefile.
#include "easing.h"
#include "../paths/paths.h"
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL  line %d: %s\n", __LINE__, #cond);          \
            ++failures;                                                   \
        }                                                                 \
    } while (0)

static bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) <= eps; }
using motion::Point2;
static double dist(Point2 a, Point2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

static void testEasings() {
    struct E { const char* name; std::function<double(double)> f; bool monotone; };
    const std::vector<E> all = {
        {"linear", easing::linear, true},
        {"easeInQuad", easing::easeInQuad, true},
        {"easeOutQuad", easing::easeOutQuad, true},
        {"easeInOutQuad", easing::easeInOutQuad, true},
        {"easeInCubic", easing::easeInCubic, true},
        {"easeOutCubic", easing::easeOutCubic, true},
        {"easeInOutCubic", easing::easeInOutCubic, true},
        {"bezier ease-in-out", [](double t) { return easing::cubicBezierEase(0.42, 0, 0.58, 1, t); }, true},
        {"bezier ease-out", [](double t) { return easing::cubicBezierEase(0, 0, 0.58, 1, t); }, true},
        {"bezier ease-in", [](double t) { return easing::cubicBezierEase(0.42, 0, 1, 1, t); }, true},
        {"bezier steep", [](double t) { return easing::cubicBezierEase(0.9, 0.0, 0.1, 1.0, t); }, true},
        {"easeOutBounce", easing::easeOutBounce, false},
        {"easeOutBack", easing::easeOutBack, false},
        {"bezier overshoot", [](double t) { return easing::cubicBezierEase(0.34, 1.56, 0.64, 1, t); }, false},
    };
    for (const auto& e : all) {
        CHECK(near(e.f(0.0), 0.0));
        CHECK(near(e.f(1.0), 1.0));
        CHECK(near(e.f(-0.5), 0.0));  // out-of-range input is clamped
        CHECK(near(e.f(1.5), 1.0));
        if (e.monotone) {
            double prev = e.f(0.0);
            for (int i = 1; i <= 1000; ++i) {
                const double v = e.f(i / 1000.0);
                CHECK(v >= prev - 1e-9);
                CHECK(v >= -1e-9 && v <= 1.0 + 1e-9);
                prev = v;
            }
        }
    }
    // Specific shapes.
    CHECK(easing::easeInQuad(0.5) < 0.5);       // slow start
    CHECK(easing::easeOutQuad(0.5) > 0.5);      // fast start
    CHECK(near(easing::easeInOutQuad(0.5), 0.5));
    // A cubic Bezier with control points on the diagonal is exactly linear.
    for (int i = 0; i <= 100; ++i)
        CHECK(near(easing::cubicBezierEase(0.25, 0.25, 0.75, 0.75, i / 100.0), i / 100.0, 1e-7));
    // Bounce stays inside [0,1] and touches 1 on its bounces; Back goes past 1.
    double maxBack = 0.0, maxBounce = 0.0, minBounce = 1.0;
    for (int i = 0; i <= 1000; ++i) {
        maxBack = std::fmax(maxBack, easing::easeOutBack(i / 1000.0));
        maxBounce = std::fmax(maxBounce, easing::easeOutBounce(i / 1000.0));
        minBounce = std::fmin(minBounce, easing::easeOutBounce(i / 1000.0));
    }
    CHECK(maxBack > 1.05 && maxBack < 1.2);
    CHECK(maxBounce <= 1.0 + 1e-12 && minBounce >= 0.0);
    std::printf("  easings ok (%zu functions checked)\n", all.size());
}

static void testPaths() {
    // Bezier: starts at P0, ends at P3, tangent at start points toward P1.
    const Point2 a{0, 0}, b{100, 200}, c{300, -100}, d{400, 50};
    CHECK(dist(motion::cubicBezier(a, b, c, d, 0.0), a) < 1e-9);
    CHECK(dist(motion::cubicBezier(a, b, c, d, 1.0), d) < 1e-9);
    const Point2 tan0 = motion::cubicBezierTangent(a, b, c, d, 0.0);
    CHECK(near(tan0.x / tan0.y, (b.x - a.x) / (b.y - a.y), 1e-9));
    // Cubic Bezier derivative matches a numerical derivative.
    const double h = 1e-6;
    const Point2 num1 = motion::cubicBezier(a, b, c, d, 0.4 + h), num0 = motion::cubicBezier(a, b, c, d, 0.4 - h);
    const Point2 tg = motion::cubicBezierTangent(a, b, c, d, 0.4);
    CHECK(near(tg.x, (num1.x - num0.x) / (2 * h), 1e-3) && near(tg.y, (num1.y - num0.y) / (2 * h), 1e-3));
    // Quadratic: midpoint of (0,0),(50,100),(100,0) is (50,50).
    const Point2 q = motion::quadBezier({0, 0}, {50, 100}, {100, 0}, 0.5);
    CHECK(near(q.x, 50) && near(q.y, 50));

    // Catmull-Rom: passes through every waypoint, exactly.
    std::vector<Point2> wp = {{10, 10}, {80, 120}, {160, 40}, {240, 160}, {300, 60}, {400, 100}};
    motion::CatmullRomPath path(wp);
    for (size_t i = 0; i < wp.size(); ++i)
        CHECK(dist(path.at(static_cast<double>(i) / (wp.size() - 1)), wp[i]) < 1e-9);
    // Smoothness: no jumps between consecutive samples.
    Point2 prev = path.at(0.0);
    double worst = 0.0;
    for (int i = 1; i <= 2000; ++i) {
        const Point2 cur = path.at(i / 2000.0);
        worst = std::fmax(worst, dist(prev, cur));
        prev = cur;
    }
    CHECK(worst < 5.0);
    // Two waypoints = a straight line.
    motion::CatmullRomPath line({{0, 0}, {100, 50}});
    CHECK(dist(line.at(0.5), {50, 25}) < 1e-9);

    // Arc length: with equal steps in u the point moves equal distances (even speed), which is
    // NOT true for equal steps in t on an uneven curve.
    auto curve = [&](double t) { return motion::cubicBezier({0, 0}, {10, 0}, {20, 0}, {300, 0}, t); };
    motion::ArcLengthTable table(curve, 400);
    CHECK(table.totalLength() > 299.0 && table.totalLength() < 301.0);  // it is a 300-long line
    std::vector<double> evenSteps, tSteps;
    for (int i = 1; i <= 10; ++i) {
        evenSteps.push_back(dist(curve(table.tAtFraction((i - 1) / 10.0)), curve(table.tAtFraction(i / 10.0))));
        tSteps.push_back(dist(curve((i - 1) / 10.0), curve(i / 10.0)));
    }
    double eMin = 1e9, eMax = 0, tMin = 1e9, tMax = 0;
    for (double v : evenSteps) { eMin = std::fmin(eMin, v); eMax = std::fmax(eMax, v); }
    for (double v : tSteps) { tMin = std::fmin(tMin, v); tMax = std::fmax(tMax, v); }
    CHECK(eMax - eMin < 0.5);            // arc-length: all steps ~30 units
    CHECK(tMax - tMin > 20.0);           // plain t: steps differ wildly
    CHECK(near(table.tAtFraction(0.0), 0.0) && near(table.tAtFraction(1.0), 1.0));
    std::printf("  paths ok (arc-length steps %.2f..%.2f vs plain-t %.2f..%.2f)\n", eMin, eMax, tMin, tMax);
}

static void testPageCurl() {
    const double W = 800, H = 600;
    auto e0 = motion::pageCurlEdge(0.0, W, H);
    auto e1 = motion::pageCurlEdge(1.0, W, H);
    for (auto& p : e0) CHECK(near(p.x, W, 1e-9));   // flat on the right at t=0
    for (auto& p : e1) CHECK(near(p.x, 0.0, 1e-9)); // flat on the left at t=1
    CHECK(near(e0.front().y, 0.0) && near(e0.back().y, H));
    auto mid = motion::pageCurlEdge(0.5, W, H);
    CHECK(mid.front().x == mid.back().x);           // top and bottom of the fold line up
    CHECK(mid[mid.size() / 2].x < mid.front().x);   // middle bows toward the turning side
    // The fold moves steadily leftwards as t grows (top point).
    double prevX = W + 1;
    for (int i = 0; i <= 20; ++i) {
        const double x = motion::pageCurlEdge(i / 20.0, W, H).front().x;
        CHECK(x < prevX);
        prevX = x;
    }
    auto region = motion::pageCurlRegion(0.5, W, H);
    CHECK(region.size() == mid.size() + 2);
    std::printf("  page curl ok\n");
}

int main() {
    std::printf("easing...\n");
    testEasings();
    std::printf("motion paths...\n");
    testPaths();
    std::printf("page curl...\n");
    testPageCurl();
    if (failures == 0) { std::printf("ALL TESTS PASSED\n"); return 0; }
    std::printf("%d CHECK(S) FAILED\n", failures);
    return 1;
}
