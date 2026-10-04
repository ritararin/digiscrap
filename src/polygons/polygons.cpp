#include "polygons/polygons.h"
#include <cmath>
Polygon makeSquare(Vec2 c, double s) { double h = s/2; return {{c.x-h,c.y-h},{c.x+h,c.y-h},{c.x+h,c.y+h},{c.x-h,c.y+h}}; }
Polygon makeStar(Vec2 c, double R, double r) {
    Polygon p;
    for (int i = 0; i < 10; i++) {
        double a = -M_PI/2 + i * M_PI/5, d = (i % 2 == 0) ? R : r;
        p.push_back({c.x + d*std::cos(a), c.y + d*std::sin(a)});
    }
    return p;
}
Polygon makeHeart(Vec2 c, double size) {
    Polygon p;
    for (int i = 0; i < 40; i++) {
        double t = 2*M_PI*i/40;
        double x = 16*std::pow(std::sin(t),3);
        double y = -(13*std::cos(t) - 5*std::cos(2*t) - 2*std::cos(3*t) - std::cos(4*t));
        p.push_back({c.x + x*size/32, c.y + y*size/32});
    }
    return p;
}
bool pointInPolygon(const Polygon&, Vec2) { return false; }   // TODO owner