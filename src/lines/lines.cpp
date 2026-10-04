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
void drawLineBresenham(Framebuffer& fb, int x0, int y0, int x1, int y1, Color c) { // TODO owner
    drawLineDDA(fb, x0, y0, x1, y1, c);
}