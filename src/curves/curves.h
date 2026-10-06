#pragma once

#include <vector>
#include "core/mat3.h"

// Generate points along a cubic Bezier curve.
Polygon bezierCubic(
    Vec2 p0,
    Vec2 p1,
    Vec2 p2,
    Vec2 p3,
    int segments = 50
);

// Generate a smooth Catmull-Rom spline through control points.
Polygon catmullRom(
    const std::vector<Vec2>& pts,
    int segmentsPerSpan = 20
);

// Scrapbook shapes created using our curve algorithms.
Polygon makeBezierHeart(
    Vec2 center,
    double size,
    int segments = 30
);

Polygon makeCloud(
    Vec2 center,
    double width,
    double height,
    int segmentsPerSpan = 15
);

Polygon makeCurvyStar(
    Vec2 center,
    double outerRadius,
    double innerRadius,
    int points = 5,
    int segmentsPerSpan = 10
);