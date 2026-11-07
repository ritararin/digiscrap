// Tests for the animation core: Mat3 additions, Transform, lerp, Timeline,
// exporter and Scene timing. Run with:  make test
#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>
#include "core/framebuffer.h"
#include "core/mat3.h"
#include "core/scene.h"
#include "animation/lerp.h"
#include "animation/timeline.h"
#include "animation/transform.h"
#include "animation/exporter.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; \
    std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)

static bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) < eps; }
static bool nearV(Vec2 a, Vec2 b, double eps = 1e-9) { return near(a.x, b.x, eps) && near(a.y, b.y, eps); }
static bool nearM(const Mat3& a, const Mat3& b, double eps = 1e-9) {
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) if (!near(a.m[i][j], b.m[i][j], eps)) return false;
    return true;
}
static bool sameColor(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }

static double polygonArea(const Polygon& p) {          // shoelace formula
    double s = 0;
    for (size_t i = 0; i < p.size(); i++) {
        Vec2 a = p[i], b = p[(i + 1) % p.size()];
        s += a.x * b.y - b.x * a.y;
    }
    return std::fabs(s) / 2.0;
}

static void testMat3() {
    // basic builders
    CHECK(nearV(Mat3::rotate(M_PI / 2).apply({1, 0}), {0, 1}));
    CHECK(nearV(Mat3::reflectX().apply({3, 2}), {3, -2}));
    CHECK(nearV(Mat3::reflectY().apply({3, 2}), {-3, 2}));
    CHECK(nearV(Mat3::reflect(M_PI / 4).apply({3, 2}), {2, 3}));      // across y = x
    CHECK(nearV(Mat3::shear(0.5, 0).apply({0, 2}), {1, 2}));
    CHECK(nearV(Mat3::translate(5, -1) * Vec2{1, 1}, {6, 0}));       // matrix * point

    // inverse: M * M^-1 = I for a messy composite
    Mat3 M = Mat3::translate(40, -25) * Mat3::rotate(0.7) * Mat3::scale(2, 0.5) * Mat3::shear(0.3, 0.1);
    CHECK(M.invertible());
    CHECK(nearM(M * M.inverse(), Mat3::identity()));
    CHECK(nearM(M.inverse() * M, Mat3::identity()));
    Vec2 p{12.5, -7.25};
    CHECK(nearV(M.inverse().apply(M.apply(p)), p));

    // reflection is its own inverse; determinant is -1
    CHECK(nearM(Mat3::reflectX().inverse(), Mat3::reflectX()));
    CHECK(near(Mat3::reflectY().determinant(), -1.0));

    // singular matrix reports itself
    CHECK(!Mat3::scale(0, 1).invertible());
}

static void testTransform() {
    // pivot stays put when position == pivot
    Transform t;
    t.pivot = t.position = {100, 50};
    t.rotation = 37; t.scale = {2, 2};
    CHECK(nearV(t.toMat3().apply({100, 50}), {100, 50}));

    // rotation about the origin pivot, then position
    Transform u;
    u.position = {10, 10}; u.rotation = 90;
    CHECK(nearV(u.toMat3().apply({1, 0}), {10, 11}));

    // rest pose reproduces the polygon exactly
    Polygon sq = {{10, 10}, {20, 10}, {20, 20}, {10, 20}};
    Polygon same = restPose(sq).apply(sq);
    for (size_t i = 0; i < sq.size(); i++) CHECK(nearV(sq[i], same[i]));

    // area scales by s^2
    Transform big = restPose(sq);
    big.scale = {3, 3}; big.rotation = 20;
    CHECK(near(polygonArea(big.apply(sq)), 9.0 * polygonArea(sq), 1e-6));

    // tween between two poses
    Transform a, b;
    b.position = {100, 0}; b.rotation = 90; b.scale = {3, 3};
    Transform mid = lerp(a, b, 0.5);
    CHECK(nearV(mid.position, {50, 0}) && near(mid.rotation, 45) && nearV(mid.scale, {2, 2}));
}

static void testLerp() {
    CHECK(lerp(2.0, 10.0, 0.0) == 2.0 && lerp(2.0, 10.0, 1.0) == 10.0);
    CHECK(near(lerp(2.0, 10.0, 0.25), 4.0));

    Vec2 a{1, 2}, b{9, -6};
    CHECK(lerp(a, b, 0).x == 1 && lerp(a, b, 1).y == -6);
    CHECK(nearV(lerp(a, b, 0.5), {5, -2}));

    Color c0{0, 10, 200, 255}, c1{255, 90, 3, 0};
    CHECK(sameColor(lerp(c0, c1, 0.0), c0));
    CHECK(sameColor(lerp(c0, c1, 1.0), c1));
    CHECK(sameColor(lerp(Color{0, 0, 0}, Color{255, 255, 255}, 0.5), Color{128, 128, 128}));
    CHECK(clamp01(-3) == 0 && clamp01(7) == 1 && clamp01(0.4) == 0.4);
}

static void testTimeline() {
    Timeline tl; tl.frames = 24;
    CHECK(tl.tAt(0) == 0.0);
    CHECK(tl.tAt(23) == 1.0);                         // exact, not 0.9999...
    for (int i = 1; i < 24; i++) CHECK(tl.tAt(i) > tl.tAt(i - 1));

    int count = 0; double last = -1;
    tl.forEach([&](int i, double t) { CHECK(i == count); count++; last = t; });
    CHECK(count == 24 && last == 1.0);

    CHECK(segmentT(0.0, 0.25, 0.75) == 0.0 && segmentT(1.0, 0.25, 0.75) == 1.0);
    CHECK(near(segmentT(0.5, 0.25, 0.75), 0.5));
}

static void testScene() {
    double seen = -1; int drawn = 0;
    Scene s;
    s.addTimed("late", 0.5, 1.0, [&](Framebuffer&, double lt) { seen = lt; drawn++; });
    Framebuffer fb(4, 4);
    s.render(fb, 0.25);  CHECK(drawn == 0);           // not visible yet
    s.render(fb, 0.75);  CHECK(drawn == 1 && near(seen, 0.5));
    s.render(fb, 1.0);   CHECK(drawn == 2 && near(seen, 1.0));

    int staticCalls = 0;
    Scene s2;
    s2.add("static", [&](Framebuffer&) { staticCalls++; });
    s2.render(fb);       CHECK(staticCalls == 1);     // old one-argument render still works
}

static void testExporter() {
    CHECK(frameFileName("output/anim", "stickers", 1)   == "output/anim/stickers_0001.bmp");
    CHECK(frameFileName("output/anim", "stickers", 123) == "output/anim/stickers_0123.bmp");

    // a solid colour survives downscaling unchanged
    Framebuffer solid(40, 30, {10, 20, 30});
    Framebuffer small = downscale(solid, 8, 6);
    CHECK(small.width == 8 && small.height == 6);
    CHECK(sameColor(small.pixels[0], Color{10, 20, 30}) && sameColor(small.pixels.back(), Color{10, 20, 30}));

    // contact sheet size: 3 columns x 2 rows of 20x10 thumbs, gap 2
    std::vector<Framebuffer> thumbs(6, Framebuffer(20, 10, {255, 0, 0}));
    Framebuffer sheet = makeContactSheet(thumbs, 3, 2, {0, 0, 0});
    CHECK(sheet.width == 3 * 20 + 4 * 2 && sheet.height == 2 * 10 + 3 * 2);

    // full export into a temporary folder
    namespace fs = std::filesystem;
    ExportOptions opt;
    opt.dir = "test_anim_out"; opt.sheetFrames = 4; opt.sheetColumns = 2; opt.thumbWidth = 16;
    Timeline tl; tl.frames = 5;
    int calls = 0; double firstT = -1, lastT = -1;
    ExportResult r = exportAnimation("fade", tl, 32, 24, {0, 0, 0},
        [&](Framebuffer& f, double t) {
            if (calls == 0) firstT = t;
            lastT = t; calls++;
            f.clear(lerp(Color{0, 0, 0}, Color{255, 255, 255}, t));
        }, opt);
    CHECK(r.ok && r.framesWritten == 5 && calls == 5);
    CHECK(firstT == 0.0 && lastT == 1.0);
    CHECK(fs::exists("test_anim_out/fade_0001.bmp") && fs::exists("test_anim_out/fade_0005.bmp"));
    CHECK(!fs::exists("test_anim_out/fade_0006.bmp"));
    CHECK(fs::exists("test_anim_out/fade_sheet.bmp"));
    fs::remove_all("test_anim_out");
}

int main() {
    testMat3();
    testTransform();
    testLerp();
    testTimeline();
    testScene();
    testExporter();
    if (failures == 0) std::cout << "All animation core tests passed.\n";
    else               std::cout << failures << " check(s) FAILED.\n";
    return failures == 0 ? 0 : 1;
}