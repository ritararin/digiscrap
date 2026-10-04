#include "circles/circles.h"
#include <cmath>
void drawCircleMidpoint(Framebuffer& fb, int cx, int cy, int, Color c) { fb.setPixel(cx, cy, c); }   // TODO owner
void drawEllipseMidpoint(Framebuffer& fb, int cx, int cy, int, int, Color c) { fb.setPixel(cx, cy, c); } // TODO owner
void fillCircle(Framebuffer& fb, int cx, int cy, int r, Color c) {                                  // TODO owner
    for (int y = -r; y <= r; y++) { int w = (int)std::sqrt((double)(r*r - y*y));
        for (int x = -w; x <= w; x++) fb.setPixel(cx + x, cy + y, c); }
}