#pragma once
#include "core/mat3.h"
Polygon bezierCubic(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, int segments = 50);
Polygon catmullRom(const std::vector<Vec2>& pts, int segmentsPerSpan = 20);