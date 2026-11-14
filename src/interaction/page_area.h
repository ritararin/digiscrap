#pragma once
#include <algorithm>
#include "core/mat3.h"

namespace interaction {

// The region of the day page where stickers may sit. The defaults match the
// limits the day page already clamps a dragged sticker's centre into.
struct PageArea {
    double x0 = 85.0,  y0 = 75.0;
    double x1 = 755.0, y1 = 525.0;

    Vec2 centre() const { return {(x0 + x1) / 2.0, (y0 + y1) / 2.0}; }

    Vec2 clamp(Vec2 p) const {
        return {std::clamp(p.x, x0, x1), std::clamp(p.y, y0, y1)};
    }
};

}  // namespace interaction
