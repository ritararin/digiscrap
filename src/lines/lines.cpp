#include "lines/lines.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
void drawLineDDA(Framebuffer& fb, int x0, int y0, int x1, int y1, Color c) {   // TODO owner
    int steps = std::max(std::abs(x1-x0), std::abs(y1-y0));
    if (!steps) { fb.setPixel(x0, y0, c); return; }
    double dx = (x1-x0)/(double)steps, dy = (y1-y0)/(double)steps, x = x0, y = y0;
    for (int i = 0; i <= steps; i++) { fb.setPixel((int)std::lround(x), (int)std::lround(y), c); x += dx; y += dy; }
}
// Bresenham's line algorithm: integer-only, all-octant formulation
void drawLineBresenham(Framebuffer& fb, int x0, int y0, int x1, int y1, Color c) {
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        fb.setPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}