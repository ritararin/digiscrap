#pragma once
#include <cassert>
#include "core/mat3.h"          // Polygon, Vec2
#include "animation/lerp.h"     // lerp(Vec2, Vec2, double)

// Per-vertex linear interpolation between two polygons of the same size.
// At t = 0 returns `a`; at t = 1 returns `b`.
inline Polygon morphPolygon(const Polygon& a, const Polygon& b, double t) {
    assert(a.size() == b.size());
    Polygon out;
    out.reserve(a.size());
    for (size_t i = 0; i < a.size(); i++)
        out.push_back(lerp(a[i], b[i], t));
    return out;
}

