#include "clipping/clipping.h"
#include <algorithm>
#include <cmath>
bool clipLine(double&, double&, double&, double&, double, double, double, double) { return true; } // TODO owner
Polygon clipPolygon(const Polygon& p, double, double, double, double) { return p; }                 // TODO owner
void fillPolygon(Framebuffer& fb, const Polygon& p, Color c) {   // TODO owner: real scanline fill
    double x0=1e9,y0=1e9,x1=-1e9,y1=-1e9;
    for (auto& v : p) { x0=std::min(x0,v.x); y0=std::min(y0,v.y); x1=std::max(x1,v.x); y1=std::max(y1,v.y); }
    for (int y = (int)y0; y <= (int)y1; y++) for (int x = (int)x0; x <= (int)x1; x++) fb.setPixel(x, y, c);
}