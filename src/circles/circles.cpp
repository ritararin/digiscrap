#include "circles/circles.h"
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <algorithm>

// Midpoint circle outline algorithm (Bresenham circle) with 8-way symmetry
void drawCircleMidpoint(Framebuffer& fb, int cx, int cy, int r, Color c) {
    if (r < 0) r = std::abs(r);
    if (r == 0) {
        fb.setPixel(cx, cy, c);
        return;
    }

    int x = 0;
    int y = r;
    int d = 1 - r;

    auto plot8 = [&](int px, int py) {
        fb.setPixel(cx + px, cy + py, c);
        fb.setPixel(cx - px, cy + py, c);
        fb.setPixel(cx + px, cy - py, c);
        fb.setPixel(cx - px, cy - py, c);
        fb.setPixel(cx + py, cy + px, c);
        fb.setPixel(cx - py, cy + px, c);
        fb.setPixel(cx + py, cy - px, c);
        fb.setPixel(cx - py, cy - px, c);
    };

    plot8(x, y);
    while (x < y) {
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            y--;
            d += 2 * (x - y) + 1;
        }
        plot8(x, y);
    }
}

// Filled circle using integer midpoint scanline spans
void fillCircle(Framebuffer& fb, int cx, int cy, int r, Color c) {
    if (r < 0) r = std::abs(r);
    if (r == 0) {
        fb.setPixel(cx, cy, c);
        return;
    }

    int x = 0;
    int y = r;
    int d = 1 - r;

    auto drawSpan = [&](int x0, int x1, int row) {
        for (int px = x0; px <= x1; px++) {
            fb.setPixel(px, row, c);
        }
    };

    while (x <= y) {
        drawSpan(cx - x, cx + x, cy + y);
        drawSpan(cx - x, cx + x, cy - y);
        drawSpan(cx - y, cx + y, cy + x);
        drawSpan(cx - y, cx + y, cy - x);
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            y--;
            d += 2 * (x - y) + 1;
        }
    }
}

// Midpoint ellipse outline algorithm with 4-way symmetry and 64-bit integer arithmetic
void drawEllipseMidpoint(Framebuffer& fb, int cx, int cy, int rx, int ry, Color c) {
    if (rx < 0) rx = std::abs(rx);
    if (ry < 0) ry = std::abs(ry);
    if (rx == 0 && ry == 0) {
        fb.setPixel(cx, cy, c);
        return;
    }
    if (rx == 0) {
        for (int y = cy - ry; y <= cy + ry; y++) fb.setPixel(cx, y, c);
        return;
    }
    if (ry == 0) {
        for (int x = cx - rx; x <= cx + rx; x++) fb.setPixel(x, cy, c);
        return;
    }

    auto plot4 = [&](int px, int py) {
        fb.setPixel(cx + px, cy + py, c);
        fb.setPixel(cx - px, cy + py, c);
        fb.setPixel(cx + px, cy - py, c);
        fb.setPixel(cx - px, cy - py, c);
    };

    int64_t a = rx;
    int64_t b = ry;
    int64_t a2 = a * a;
    int64_t b2 = b * b;

    // Region 1: slope dy/dx > -1 (step x by 1)
    int64_t x = 0;
    int64_t y = b;
    int64_t dx = 2 * b2 * x;
    int64_t dy = 2 * a2 * y;
    int64_t d1 = b2 - (a2 * b) + (a2 + 2) / 4;

    plot4(static_cast<int>(x), static_cast<int>(y));
    while (dx < dy) {
        x++;
        dx += 2 * b2;
        if (d1 < 0) {
            d1 += dx + b2;
        } else {
            y--;
            dy -= 2 * a2;
            d1 += dx - dy + b2;
        }
        plot4(static_cast<int>(x), static_cast<int>(y));
    }

    // Region 2: slope dy/dx < -1 (step y by -1)
    int64_t d2 = b2 * (x * x + x) + (b2 + 2) / 4 + a2 * (y - 1) * (y - 1) - a2 * b2;
    while (y > 0) {
        y--;
        dy -= 2 * a2;
        if (d2 > 0) {
            d2 += a2 - dy;
        } else {
            x++;
            dx += 2 * b2;
            d2 += dx - dy + a2;
        }
        plot4(static_cast<int>(x), static_cast<int>(y));
    }
}

// Filled ellipse using midpoint scanline spans
void fillEllipse(Framebuffer& fb, int cx, int cy, int rx, int ry, Color c) {
    if (rx < 0) rx = std::abs(rx);
    if (ry < 0) ry = std::abs(ry);
    if (rx == 0 && ry == 0) {
        fb.setPixel(cx, cy, c);
        return;
    }
    if (rx == 0) {
        for (int y = cy - ry; y <= cy + ry; y++) fb.setPixel(cx, y, c);
        return;
    }
    if (ry == 0) {
        for (int x = cx - rx; x <= cx + rx; x++) fb.setPixel(x, cy, c);
        return;
    }

    auto drawSpan = [&](int px, int py) {
        for (int i = cx - px; i <= cx + px; i++) {
            fb.setPixel(i, cy + py, c);
            fb.setPixel(i, cy - py, c);
        }
    };

    int64_t a = rx;
    int64_t b = ry;
    int64_t a2 = a * a;
    int64_t b2 = b * b;

    int64_t x = 0;
    int64_t y = b;
    int64_t dx = 2 * b2 * x;
    int64_t dy = 2 * a2 * y;
    int64_t d1 = b2 - (a2 * b) + (a2 + 2) / 4;

    drawSpan(static_cast<int>(x), static_cast<int>(y));
    while (dx < dy) {
        x++;
        dx += 2 * b2;
        if (d1 < 0) {
            d1 += dx + b2;
        } else {
            y--;
            dy -= 2 * a2;
            d1 += dx - dy + b2;
        }
        drawSpan(static_cast<int>(x), static_cast<int>(y));
    }

    int64_t d2 = b2 * (x * x + x) + (b2 + 2) / 4 + a2 * (y - 1) * (y - 1) - a2 * b2;
    while (y > 0) {
        y--;
        dy -= 2 * a2;
        if (d2 > 0) {
            d2 += a2 - dy;
        } else {
            x++;
            dx += 2 * b2;
            d2 += dx - dy + a2;
        }
        drawSpan(static_cast<int>(x), static_cast<int>(y));
    }
}