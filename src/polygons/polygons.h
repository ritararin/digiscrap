#pragma once
#include "core/mat3.h"
Polygon makeStar(Vec2 c, double outer, double inner);
Polygon makeHeart(Vec2 c, double size);
Polygon makeSquare(Vec2 c, double side);
bool pointInPolygon(const Polygon&, Vec2 p);