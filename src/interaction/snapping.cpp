#include "interaction/snapping.h"
#include <cmath>
#include "polygons/polygons.h"      // boundingBox
#include "animation/transform.h"

namespace interaction {

namespace {

struct Box { double x0, y0, x1, y1; };

// Axis-aligned bounding box of the sticker's world polygon with its centre at `position`.
Box worldBox(const SavedElement& e, Vec2 position) {
    Transform xf = e.xform;
    xf.position = position;
    const Polygon world = transformPolygon(buildShapePolygon(e.kind), xf.toMat3());
    Vec2 mn, mx;
    boundingBox(world, mn, mx);
    return {mn.x, mn.y, mx.x, mx.y};
}

struct Best {
    bool   found  = false;
    double delta  = 0.0;                        // how far to move the sticker
    double target = 0.0;                        // the line it lands on (the guide)
};

void consider(Best& b, double feature, double target, double threshold) {
    const double d = target - feature;
    if (std::fabs(d) > threshold) return;
    if (!b.found || std::fabs(d) < std::fabs(b.delta)) {
        b.found = true;
        b.delta = d;
        b.target = target;
    }
}

}  // namespace

SnapResult snapElement(const std::vector<SavedElement>& all, int movingIndex, Vec2 desired,
                       const PageArea& area, double threshold) {
    SnapResult r;
    r.position = desired;
    if (movingIndex < 0 || movingIndex >= (int)all.size()) return r;

    const Box m = worldBox(all[movingIndex], desired);
    // Centre lines come first so that, on a tie, the guide runs through the middle.
    const double fx[3] = {(m.x0 + m.x1) / 2.0, m.x0, m.x1};
    const double fy[3] = {(m.y0 + m.y1) / 2.0, m.y0, m.y1};

    Best bx, by;
    auto targetX = [&](double t) { for (double f : fx) consider(bx, f, t, threshold); };
    auto targetY = [&](double t) { for (double f : fy) consider(by, f, t, threshold); };

    targetX(area.centre().x);  targetX(area.x0);  targetX(area.x1);
    targetY(area.centre().y);  targetY(area.y0);  targetY(area.y1);

    for (int j = 0; j < (int)all.size(); j++) {
        if (j == movingIndex) continue;
        const Box o = worldBox(all[j], all[j].xform.position);
        targetX((o.x0 + o.x1) / 2.0);  targetX(o.x0);  targetX(o.x1);
        targetY((o.y0 + o.y1) / 2.0);  targetY(o.y0);  targetY(o.y1);
    }

    if (bx.found) { r.position.x += bx.delta; r.guides.hasX = true; r.guides.x = bx.target; }
    if (by.found) { r.position.y += by.delta; r.guides.hasY = true; r.guides.y = by.target; }
    return r;
}

}  // namespace interaction
