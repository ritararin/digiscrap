#include "curves/curves.h"
Polygon bezierCubic(Vec2 a, Vec2, Vec2, Vec2 d, int) { return {a, d}; }          // TODO owner
Polygon catmullRom(const std::vector<Vec2>& pts, int) { return pts; }            // TODO owner

