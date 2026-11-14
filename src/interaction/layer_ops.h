#pragma once
// Duplicate and layer-order operations on a day's sticker list (CP3, Option 4).
//
// Like DayLayout::sortByZ(), the list is kept in drawing order: index 0 is drawn
// first (at the back), the last index is drawn last (on top).
#include <vector>
#include "calendar/day_storage.h"   // SavedElement
#include "interaction/page_area.h"

namespace interaction {

// Copy elems[index], nudge the copy by `offset` (kept inside `area`) and put it on top.
// Returns the copy's index (always the last one), or -1 if `index` is invalid.
int duplicateElement(std::vector<SavedElement>& elems, int index, const PageArea& area,
                     Vec2 offset = {24.0, 24.0});

// Move elems[index] by `delta` layers (+1 = one layer closer to the front, -1 = one
// layer back). Moves past either end stop at the end. zOrder is renumbered 0..n-1 so
// the order is always unambiguous. Returns the element's new index, or -1 if invalid.
int moveLayer(std::vector<SavedElement>& elems, int index, int delta);

}  // namespace interaction
