#include "lines/lines.h"
#include "circles/circles.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

static int failures = 0;
#define CHECK(cond, name) do { \
    if (cond) std::cout << "PASS: " << name << "\n"; \
    else { std::cout << "FAIL: " << name << "\n"; failures++; } } while (0)

static int countPixels(const Framebuffer& fb, Color target) {
    int count = 0;
    for (const auto& p : fb.pixels) {
        if (p.r == target.r && p.g == target.g && p.b == target.b) count++;
    }
    return count;
}

int main() {
    std::cout << "--- Lines: Bresenham & DDA Tests ---\n";
    {
        Framebuffer fb(100, 100);
        Color red{255, 0, 0};

        // Horizontal line
        fb.clear({0,0,0});
        drawLineBresenham(fb, 10, 20, 30, 20, red);
        CHECK(countPixels(fb, red) == 21, "Bresenham: horizontal line length 21");

        // Vertical line
        fb.clear({0,0,0});
        drawLineBresenham(fb, 20, 10, 20, 30, red);
        CHECK(countPixels(fb, red) == 21, "Bresenham: vertical line length 21");

        // Diagonal 45-degree
        fb.clear({0,0,0});
        drawLineBresenham(fb, 10, 10, 30, 30, red);
        CHECK(countPixels(fb, red) == 21, "Bresenham: diagonal 45-deg line length 21");

        // Degenerate single point
        fb.clear({0,0,0});
        drawLineBresenham(fb, 15, 15, 15, 15, red);
        CHECK(countPixels(fb, red) == 1, "Bresenham: single point line length 1");

        // All 8 octants test
        int dx_tests[8] = {20,  10, -10, -20, -20, -10,  10,  20};
        int dy_tests[8] = {10,  20,  20,  10, -10, -20, -20, -10};
        bool allOctantsPass = true;
        for (int i = 0; i < 8; i++) {
            fb.clear({0,0,0});
            int x0 = 50, y0 = 50;
            int x1 = x0 + dx_tests[i], y1 = y0 + dy_tests[i];
            drawLineBresenham(fb, x0, y0, x1, y1, red);
            int expected = std::max(std::abs(dx_tests[i]), std::abs(dy_tests[i])) + 1;
            int actual = countPixels(fb, red);
            if (actual != expected) allOctantsPass = false;
        }
        CHECK(allOctantsPass, "Bresenham: correctly covers all 8 octants with exact pixel count");
    }

    std::cout << "\n--- Circles: Midpoint Outline & Fill Tests ---\n";
    {
        Framebuffer fb(120, 120);
        Color blue{0, 0, 255};

        // Midpoint Circle Outline
        fb.clear({0,0,0});
        int cx = 60, cy = 60, r = 30;
        drawCircleMidpoint(fb, cx, cy, r, blue);

        // 8-way symmetry check
        bool symmetryPass = true;
        for (int y = 0; y < 120; y++) {
            for (int x = 0; x < 120; x++) {
                Color p = fb.pixels[y * 120 + x];
                if (p.r == blue.r && p.g == blue.g && p.b == blue.b) {
                    int dx = std::abs(x - cx);
                    int dy = std::abs(y - cy);
                    // Point must lie close to radius
                    double dist = std::sqrt(dx*dx + dy*dy);
                    if (std::abs(dist - r) > 1.2) symmetryPass = false;
                    // Opposite symmetric points must also be set
                    Color s1 = fb.pixels[(cy + dy) * 120 + (cx + dx)];
                    Color s2 = fb.pixels[(cy - dy) * 120 + (cx + dx)];
                    Color s3 = fb.pixels[(cy + dy) * 120 + (cx - dx)];
                    Color s4 = fb.pixels[(cy - dy) * 120 + (cx - dx)];
                    if (s1.b != 255 || s2.b != 255 || s3.b != 255 || s4.b != 255) symmetryPass = false;
                }
            }
        }
        CHECK(symmetryPass, "Midpoint circle: 8-way symmetry and radial distance");

        // Fill Circle Check
        fb.clear({0,0,0});
        fillCircle(fb, cx, cy, r, blue);
        int filledCount = countPixels(fb, blue);
        double expectedArea = 3.1415926535 * r * r;
        CHECK(std::abs(filledCount - expectedArea) / expectedArea < 0.03,
              "fillCircle: pixel count matches pi * r^2 within 3%");

        // Center must be filled
        Color centerPix = fb.pixels[cy * 120 + cx];
        CHECK(centerPix.b == 255, "fillCircle: center pixel is filled");

        // Zero radius
        fb.clear({0,0,0});
        fillCircle(fb, cx, cy, 0, blue);
        CHECK(countPixels(fb, blue) == 1, "fillCircle: r=0 fills single center pixel");
    }

    std::cout << "\n--- Ellipses: Midpoint Outline & Fill Tests ---\n";
    {
        Framebuffer fb(120, 120);
        Color green{0, 255, 0};

        int cx = 60, cy = 60, rx = 40, ry = 25;
        drawEllipseMidpoint(fb, cx, cy, rx, ry, green);

        // 4-way symmetry and ellipse equation check
        bool ellipsePass = true;
        for (int y = 0; y < 120; y++) {
            for (int x = 0; x < 120; x++) {
                Color p = fb.pixels[y * 120 + x];
                if (p.r == green.r && p.g == green.g && p.b == green.b) {
                    int dx = std::abs(x - cx);
                    int dy = std::abs(y - cy);
                    double val = (double)(dx * dx) / (rx * rx) + (double)(dy * dy) / (ry * ry);
                    if (std::abs(val - 1.0) > 0.15) ellipsePass = false;
                    // 4-way symmetry check
                    Color s1 = fb.pixels[(cy + dy) * 120 + (cx + dx)];
                    Color s2 = fb.pixels[(cy - dy) * 120 + (cx + dx)];
                    Color s3 = fb.pixels[(cy + dy) * 120 + (cx - dx)];
                    Color s4 = fb.pixels[(cy - dy) * 120 + (cx - dx)];
                    if (s1.g != 255 || s2.g != 255 || s3.g != 255 || s4.g != 255) ellipsePass = false;
                }
            }
        }
        CHECK(ellipsePass, "Midpoint ellipse: 4-way symmetry and algebraic equation fit");

        // Fill Ellipse Check
        fb.clear({0,0,0});
        fillEllipse(fb, cx, cy, rx, ry, green);
        int filledCount = countPixels(fb, green);
        double expectedArea = 3.1415926535 * rx * ry;
        CHECK(std::abs(filledCount - expectedArea) / expectedArea < 0.03,
              "fillEllipse: pixel count matches pi * rx * ry within 3%");
    }

    std::cout << (failures == 0 ? "\nALL CHECKS PASSED\n" : "\nSOME CHECKS FAILED\n");
    return failures;
}
