#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <iostream>
#include <string>
#include <cstdlib>

#include "core/mat3.h"
#include "opengl/renderer.h"

static int passed = 0, failed = 0;

static void check(bool cond, const std::string& label) {
    if (cond) { std::cout << "PASS: " << label << "\n"; ++passed; }
    else      { std::cout << "FAIL: " << label << "\n"; ++failed; }
}

static bool nearVal(float a, float b, float tol = 1e-4f) {
    return std::fabs(a - b) < tol;
}

// -----------------------------------------------------------------------
// Test 1: Identity Mat3 converts to 4x4 identity
// -----------------------------------------------------------------------
static void testIdentity() {
    Mat3 m = Mat3::identity();
    float m4[16];
    gl::mat3ToMat4(m, m4);

    bool ok = true;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float expected = (col == row) ? 1.0f : 0.0f;
            if (!nearVal(m4[col * 4 + row], expected))
                ok = false;
        }
    }
    check(ok, "Identity Mat3 converts to 4x4 identity");
}

// -----------------------------------------------------------------------
// Test 2: Translation Mat3 to 4x4
// -----------------------------------------------------------------------
static void testTranslation() {
    Mat3 m = Mat3::translate(150.0, -250.0);
    float m4[16];
    gl::mat3ToMat4(m, m4);

    // In OpenGL column-major float[16]:
    // col3 is m4[12..15] -> tx = m4[12], ty = m4[13], tz = m4[14], tw = m4[15]
    check(nearVal(m4[12], 150.0f) && nearVal(m4[13], -250.0f) &&
          nearVal(m4[14], 0.0f)   && nearVal(m4[15], 1.0f),
          "Translation tx, ty in column 3 (m4[12], m4[13])");
}

// -----------------------------------------------------------------------
// Test 3: Scale Mat3 to 4x4
// -----------------------------------------------------------------------
static void testScale() {
    Mat3 m = Mat3::scale(2.5, 0.75);
    float m4[16];
    gl::mat3ToMat4(m, m4);

    check(nearVal(m4[0], 2.5f) && nearVal(m4[5], 0.75f) && nearVal(m4[10], 1.0f),
          "Scale sx, sy on diagonal (m4[0], m4[5])");
}

// -----------------------------------------------------------------------
// Test 4: Equivalence of Mat3::apply(Vec2) and 4x4 vector multiplication
// -----------------------------------------------------------------------
static void testTransformEquivalence() {
    // Composite transform: translate, rotate, scale
    Mat3 m = Mat3::translate(200, 100) * Mat3::rotate(0.5) * Mat3::scale(1.5, 2.0);
    float m4[16];
    gl::mat3ToMat4(m, m4);

    Vec2 p{40.0, 70.0};
    Vec2 pTrans = m.apply(p);

    // Compute via 4x4: v' = M4 * (x, y, 0, 1)
    float x4 = m4[0]*p.x + m4[4]*p.y + m4[8]*0.0f + m4[12]*1.0f;
    float y4 = m4[1]*p.x + m4[5]*p.y + m4[9]*0.0f + m4[13]*1.0f;

    check(nearVal((float)pTrans.x, x4) && nearVal((float)pTrans.y, y4),
          "Mat3 2D point transform matches 4x4 vector multiplication");
}

// -----------------------------------------------------------------------
// Test 5: Orthographic projection matrix
// -----------------------------------------------------------------------
static void testOrthoProjection() {
    float ortho[16];
    gl::orthoMatrix(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f, ortho);

    // Top-left pixel (0, 0): should map to NDC (-1, 1)
    float xTL = ortho[0] * 0.0f + ortho[12];
    float yTL = ortho[5] * 0.0f + ortho[13];
    check(nearVal(xTL, -1.0f) && nearVal(yTL, 1.0f),
          "Top-left (0, 0) maps to NDC (-1, 1)");

    // Bottom-right pixel (800, 600): should map to NDC (1, -1)
    float xBR = ortho[0] * 800.0f + ortho[12];
    float yBR = ortho[5] * 600.0f + ortho[13];
    check(nearVal(xBR, 1.0f) && nearVal(yBR, -1.0f),
          "Bottom-right (800, 600) maps to NDC (1, -1)");

    // Centre pixel (400, 300): should map to NDC (0, 0)
    float xC = ortho[0] * 400.0f + ortho[12];
    float yC = ortho[5] * 300.0f + ortho[13];
    check(nearVal(xC, 0.0f) && nearVal(yC, 0.0f),
          "Center (400, 300) maps to NDC (0, 0)");
}

// -----------------------------------------------------------------------

int main() {
    std::cout << "--- OpenGL Core & Matrix Tests ---\n";
    testIdentity();
    testTranslation();
    testScale();
    testTransformEquivalence();
    testOrthoProjection();

    std::cout << "\n" << passed << " passed, " << failed << " failed.\n";
    if (failed == 0) std::cout << "ALL CHECKS PASSED\n";
    return failed ? 1 : 0;
}

