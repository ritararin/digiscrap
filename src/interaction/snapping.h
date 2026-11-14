#pragma once
// Snapping and alignment guides while a sticker is dragged (CP3, Option 4).
//
// The dragged sticker's bounding box (left / centre / right and top / middle /
// bottom) is compared with the same lines of every other sticker and of the page
// area. If a pair is within `threshold` pixels, the sticker is nudged so the two
// lines coincide, and the matching guide line is reported so it can be drawn.
#include <vector>
#include "calendar/day_storage.h"   // SavedElement
#include "interaction/page_area.h"

namespace interaction {

struct SnapGuides {
    bool   hasX = false, hasY = false;
    double x = 0.0, y = 0.0;                    // where to draw the vertical / horizontal guide
};

struct SnapResult {
    Vec2       position;                        // corrected centre position for the dragged sticker
    SnapGuides guides;
};

constexpr double kDefaultSnapThreshold = 6.0;   // pixels

// all[movingIndex] is the sticker being dragged and `desired` is where the mouse
// would put its position. Returns `desired` unchanged when nothing is close enough
// or movingIndex is invalid.
SnapResult snapElement(const std::vector<SavedElement>& all, int movingIndex, Vec2 desired,
                       const PageArea& area, double threshold = kDefaultSnapThreshold);

}  // namespace interaction
