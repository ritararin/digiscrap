#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <cassert>
#include <iostream>
#include <string>
#include <cstdlib>

#include "core/mat3.h"
#include "core/framebuffer.h"
#include "animation/lerp.h"
#include "animation/transform.h"
#include "polygons/polygons.h"
#include "stickers/sticker.h"
#include "stickers/morph.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ------------------------------------------------------------------
static int passed = 0, failed = 0;

static void check(bool cond, const std::string& label) {
    if (cond) { std::cout << "PASS: " << label << "\n"; ++passed; }
    else      { std::cout << "FAIL: " << label << "\n"; ++failed; }
}

// Approximate equality helpers.
static bool near(double a, double b, double tol = 1e-3) { return std::fabs(a - b) < tol; }
static bool nearVec(Vec2 a, Vec2 b, double tol = 1e-3) {
    return near(a.x, b.x, tol) && near(a.y, b.y, tol);
}

// ------------------------------------------------------------------
// Test 1: Transformed area scales by s^2
// ------------------------------------------------------------------
static void testAreaScaling() {
    Polygon base = makeStar({0, 0}, 50, 22);
    double baseArea = polygonArea(base);

    for (double s : {0.5, 1.0, 1.5, 2.0, 3.0}) {
        Sticker st = makeSticker(base, {100, 100}, {}, {}, 0);
        st.xform.scale = {s, s};
        double expected = baseArea * s * s;

        // Method 1: Sticker::area() uses analytic formula.
        check(near(st.area(), expected, expected * 0.001),
              "area() analytic, s=" + std::to_string(s));

        // Method 2: Compute area from the actual transformed vertices.
        double wpArea = polygonArea(st.worldPolygon());
        check(near(wpArea, expected, expected * 0.002),
              "worldPolygon area, s=" + std::to_string(s));
    }
}

// ------------------------------------------------------------------
// Test 2: Non-uniform scale area (sx * sy)
// ------------------------------------------------------------------
static void testNonUniformScaleArea() {
    Polygon base = makeSquare({0, 0}, 100);  // area = 10000
    Sticker st = makeSticker(base, {200, 200}, {}, {}, 0);
    st.xform.scale = {2.0, 0.5};
    double expected = 10000.0 * 2.0 * 0.5;   // 10000
    check(near(st.area(), expected, 1.0), "non-uniform scale area (2, 0.5)");
    check(near(polygonArea(st.worldPolygon()), expected, 5.0),
          "non-uniform scale worldPolygon area");
}

// ------------------------------------------------------------------
// Test 3: Hit test at centroid stays correct after rotation
// ------------------------------------------------------------------
static void testHitTestAfterRotation() {
    Polygon base = makeStar({0, 0}, 50, 22);

    for (double deg : {0.0, 45.0, 90.0, 135.0, 180.0, 270.0}) {
        Sticker st = makeSticker(base, {300, 300}, {}, {}, 0);
        st.xform.rotation = deg;
        // The centroid of the sticker in world space should be near `position`
        // because pivot == original centroid and position is where it maps to.
        check(st.hitTest(st.xform.position),
              "centroid hit after " + std::to_string((int)deg) + " deg rotation");
    }
}

// ------------------------------------------------------------------
// Test 4: Hit test outside after rotation
// ------------------------------------------------------------------
static void testHitTestOutsideAfterRotation() {
    Polygon base = makeSquare({0, 0}, 60);  // 60×60 square
    Sticker st = makeSticker(base, {300, 300}, {}, {}, 0);
    st.xform.rotation = 45.0;

    // A point far away should always miss.
    check(!st.hitTest({0, 0}), "far point misses after 45 deg rotation");
    check(!st.hitTest({600, 600}), "far point (600,600) misses after 45 deg rotation");
}

// ------------------------------------------------------------------
// Test 5: Hit test with non-uniform scale
// ------------------------------------------------------------------
static void testHitTestNonUniformScale() {
    Polygon base = makeSquare({0, 0}, 40);  // 40×40 centred at origin
    Sticker st = makeSticker(base, {200, 200}, {}, {}, 0);
    st.xform.scale = {3.0, 0.5};  // stretched horizontally, squished vertically

    // Centre should hit.
    check(st.hitTest({200, 200}), "centre hits with scale (3, 0.5)");
    // Point along the stretched x axis, within half-width*3 = 60 of centre.
    check(st.hitTest({250, 200}), "stretched x-axis hits");
    // Point beyond the squished y extent (half-height*0.5 = 10).
    check(!st.hitTest({200, 220}), "beyond squished y-axis misses");
}

// ------------------------------------------------------------------
// Test 6: Inverse round-trip  M * M^-1 ≈ I
// ------------------------------------------------------------------
static void testInverseRoundTrip() {
    Sticker st = makeSticker(makeStar({0, 0}, 50, 22), {123, 456}, {}, {}, 0);
    st.xform.rotation = 37.0;
    st.xform.scale    = {1.8, 0.7};

    Mat3 M    = st.forwardMatrix();
    Mat3 Minv = st.inverseMatrix();
    Mat3 I    = M * Minv;

    bool ok = true;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            double expected = (i == j) ? 1.0 : 0.0;
            if (!near(I.m[i][j], expected, 1e-9)) ok = false;
        }
    check(ok, "M * M^-1 ≈ identity");
}

// ------------------------------------------------------------------
// Test 7: Z-order pick priority
// ------------------------------------------------------------------
static void testZOrderPick() {
    // Two overlapping stickers centred at the same spot.
    Polygon sq = makeSquare({0, 0}, 80);
    Sticker lo = makeSticker(sq, {200, 200}, {}, {}, 1);
    Sticker hi = makeSticker(sq, {200, 200}, {}, {}, 5);

    StickerScene scene;
    scene.add(lo);
    scene.add(hi);
    scene.sortByZ();

    Sticker* picked = scene.pickAt({200, 200});
    check(picked != nullptr && picked->zOrder == 5,
          "pickAt returns higher-z sticker");
}

// ------------------------------------------------------------------
// Test 8: bringToFront
// ------------------------------------------------------------------
static void testBringToFront() {
    Polygon sq = makeSquare({0, 0}, 80);
    Sticker a = makeSticker(sq, {200, 200}, {100, 0, 0}, {}, 1);
    Sticker b = makeSticker(sq, {200, 200}, {0, 100, 0}, {}, 5);
    Sticker c = makeSticker(sq, {200, 200}, {0, 0, 100}, {}, 3);

    StickerScene scene;
    scene.add(a);
    scene.add(b);
    scene.add(c);
    scene.sortByZ();

    // Bring 'a' (originally z=1) to front.
    scene.bringToFront(scene.stickers[0]);  // after sort, stickers[0] has z=1

    // After bringToFront, the brought sticker should be last (highest z).
    Sticker& last = scene.stickers.back();
    check(last.fill.r == 100 && last.zOrder == 6,
          "bringToFront moves sticker to highest z");

    Sticker* picked = scene.pickAt({200, 200});
    check(picked != nullptr && picked->fill.r == 100,
          "bringToFront sticker is now picked first");
}

// ------------------------------------------------------------------
// Test 9: Morph at t=0 and t=1
// ------------------------------------------------------------------
static void testMorphEndpoints() {
    Polygon star    = makeStar({0, 0}, 50, 22);
    Polygon decagon = makeRegularPolygon({0, 0}, 50, 10);

    Polygon at0 = morphPolygon(star, decagon, 0.0);
    Polygon at1 = morphPolygon(star, decagon, 1.0);

    bool ok0 = true, ok1 = true;
    for (size_t i = 0; i < star.size(); i++) {
        if (!nearVec(at0[i], star[i]))    ok0 = false;
        if (!nearVec(at1[i], decagon[i])) ok1 = false;
    }
    check(ok0, "morph(t=0) == source polygon");
    check(ok1, "morph(t=1) == target polygon");
}

// ------------------------------------------------------------------
// Test 10: Morph area continuity (convex → convex is monotone)
// ------------------------------------------------------------------
static void testMorphAreaContinuity() {
    // Both convex, same vertex count, so area should change smoothly.
    Polygon a = makeRegularPolygon({0, 0}, 40, 10);
    Polygon b = makeRegularPolygon({0, 0}, 80, 10);
    double areaA = polygonArea(a);
    double areaB = polygonArea(b);

    bool monotone = true;
    double prev = areaA;
    for (int i = 1; i <= 20; i++) {
        double t = i / 20.0;
        double area = polygonArea(morphPolygon(a, b, t));
        if (area < prev - 1e-6) monotone = false;  // should be non-decreasing
        prev = area;
    }
    check(monotone, "morph area is monotone (small → large convex)");
    check(near(prev, areaB, areaB * 0.001), "morph final area matches target");
}

// ------------------------------------------------------------------

int main() {
    testAreaScaling();
    testNonUniformScaleArea();
    testHitTestAfterRotation();
    testHitTestOutsideAfterRotation();
    testHitTestNonUniformScale();
    testInverseRoundTrip();
    testZOrderPick();
    testBringToFront();
    testMorphEndpoints();
    testMorphAreaContinuity();

    std::cout << "\n" << passed << " passed, " << failed << " failed.\n";
    if (failed == 0) std::cout << "ALL CHECKS PASSED\n";
    return failed ? 1 : 0;
}

