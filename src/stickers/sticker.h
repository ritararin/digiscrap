#pragma once
#include <vector>
#include <algorithm>
#include <functional>
#include "core/mat3.h"
#include "core/framebuffer.h"
#include "animation/transform.h"
#include "polygons/polygons.h"
#include "lines/lines.h"

// ---------------------------------------------------------------------------
// Sticker: a polygon with a transform, colour and z-order.
// The pivot is always the centroid of the base (rest-pose) shape.
// ---------------------------------------------------------------------------

struct Sticker {
    Polygon baseShape;          // vertices in rest-pose coordinates
    Transform xform;            // position, rotation, scale, pivot
    Color fill  {200, 200, 200};
    Color edge  {60, 60, 60};
    int zOrder  = 0;

    // The polygon in screen (world) space.
    Polygon worldPolygon() const { return xform.apply(baseShape); }

    // Forward and inverse composite matrix.
    Mat3 forwardMatrix() const { return xform.toMat3(); }
    Mat3 inverseMatrix() const { return xform.toMat3().inverse(); }

    // Hit test: map the click into rest-pose space and test the base shape.
    bool hitTest(Vec2 click) const {
        Vec2 local = inverseMatrix() * click;
        return pointInPolygon(baseShape, local);
    }

    // Transformed area = base area * |det(M)| = base area * sx * sy
    // (rotation and translation do not change area).
    double area() const { return polygonArea(baseShape) * std::fabs(xform.scale.x * xform.scale.y); }
};

// Convenience factory: pivot and position both start at the centroid.
inline Sticker makeSticker(const Polygon& shape, Vec2 pos, Color fill, Color edgeCol, int z) {
    Sticker s;
    s.baseShape = shape;
    s.fill      = fill;
    s.edge      = edgeCol;
    s.zOrder    = z;
    s.xform     = restPose(shape);   // pivot = centroid
    s.xform.position = pos;          // place it where the caller wants
    return s;
}

// ---------------------------------------------------------------------------
// StickerScene: z-ordered collection of stickers with picking.
// ---------------------------------------------------------------------------

class StickerScene {
public:
    std::vector<Sticker> stickers;

    void add(const Sticker& s) { stickers.push_back(s); }

    // Stable-sort so that equal-z stickers keep insertion order.
    void sortByZ() {
        std::stable_sort(stickers.begin(), stickers.end(),
                         [](const Sticker& a, const Sticker& b) { return a.zOrder < b.zOrder; });
    }

    // Render bottom-to-top (painter's algorithm).
    void render(Framebuffer& fb) const {
        for (const auto& s : stickers) {
            Polygon wp = s.worldPolygon();
            fillPolygon(fb, wp, s.fill);
            for (size_t i = 0; i < wp.size(); i++) {
                Vec2 a = wp[i], b = wp[(i + 1) % wp.size()];
                drawLineBresenham(fb, (int)a.x, (int)a.y, (int)b.x, (int)b.y, s.edge);
            }
        }
    }

    // Pick the top-most (highest z) sticker at `click`. Returns nullptr if none hit.
    Sticker* pickAt(Vec2 click) {
        // Walk back-to-front (highest z last in sorted order → iterate in reverse).
        for (int i = (int)stickers.size() - 1; i >= 0; i--) {
            if (stickers[i].hitTest(click))
                return &stickers[i];
        }
        return nullptr;
    }

    // Move `s` in front of everything else.
    void bringToFront(Sticker& target) {
        int maxZ = 0;
        for (const auto& s : stickers)
            maxZ = std::max(maxZ, s.zOrder);
        target.zOrder = maxZ + 1;
        sortByZ();
    }
};

