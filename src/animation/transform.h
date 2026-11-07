#pragma once
#include "core/mat3.h"
#include "animation/lerp.h"

// Position, rotation, scale and pivot for one object (sticker, photo, ...).
//
//   pivot     : point in the object's own (rest-pose) coordinates that
//               rotation and scale happen around (usually the centroid)
//   position  : where the pivot ends up on the page
//   rotation  : degrees, clockwise on screen (y points down)
//   scale     : x / y scale factors about the pivot
//
// toMat3() = T(position) * R(rotation) * S(scale) * T(-pivot)
// A polygon drawn at its rest pose has position == pivot, rotation 0, scale 1.
struct Transform {
    Vec2   position{0, 0};
    double rotation = 0.0;
    Vec2   scale{1, 1};
    Vec2   pivot{0, 0};

    Mat3 toMat3() const {
        const double rad = rotation * M_PI / 180.0;
        return Mat3::translate(position.x, position.y)
             * Mat3::rotate(rad)
             * Mat3::scale(scale.x, scale.y)
             * Mat3::translate(-pivot.x, -pivot.y);
    }

    Polygon apply(const Polygon& p) const { return transformPolygon(p, toMat3()); }
};

inline Vec2 polygonCentroid(const Polygon& p) {
    Vec2 c{0, 0};
    if (p.empty()) return c;
    for (auto& v : p) { c.x += v.x; c.y += v.y; }
    return { c.x / p.size(), c.y / p.size() };
}

// Rest pose for a polygon: pivot and position both at its centroid.
inline Transform restPose(const Polygon& p) {
    Transform t;
    t.pivot = t.position = polygonCentroid(p);
    return t;
}

// Interpolate every field (used to tween a sticker between two poses).
inline Transform lerp(const Transform& a, const Transform& b, double t) {
    Transform r;
    r.position = lerp(a.position, b.position, t);
    r.rotation = lerp(a.rotation, b.rotation, t);
    r.scale    = lerp(a.scale,    b.scale,    t);
    r.pivot    = lerp(a.pivot,    b.pivot,    t);
    return r;
}