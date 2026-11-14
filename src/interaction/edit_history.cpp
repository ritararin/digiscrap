#include "interaction/edit_history.h"

namespace interaction {

namespace {

bool sameColor(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }

bool sameVec(Vec2 a, Vec2 b) { return a.x == b.x && a.y == b.y; }

// zOrder is deliberately NOT compared. The element list is kept in drawing order, so the
// list order already carries the layering; comparing the raw numbers would make a click on
// a sticker that is already on top (which bumps its zOrder) look like an edit.
bool sameElement(const SavedElement& a, const SavedElement& b) {
    return a.kind == b.kind
        && sameVec(a.xform.position, b.xform.position)
        && a.xform.rotation == b.xform.rotation
        && sameVec(a.xform.scale, b.xform.scale)
        && sameVec(a.xform.pivot, b.xform.pivot)
        && sameColor(a.fill, b.fill)
        && sameColor(a.edge, b.edge);
}

}  // namespace

bool sameElements(const std::vector<SavedElement>& a, const std::vector<SavedElement>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); i++)
        if (!sameElement(a[i], b[i])) return false;
    return true;
}

void EditHistory::commit(const DayLayout& layout, bool gestureInProgress) {
    const DayKey k{layout.year, layout.month, layout.day};

    if (!hasBase_ || !(k == key_)) {            // new day (or first call): start over
        undo_.clear();
        redo_.clear();
        base_    = layout.elements;
        key_     = k;
        hasBase_ = true;
        return;
    }
    if (gestureInProgress) return;              // a drag is still running
    if (sameElements(base_, layout.elements)) return;

    undo_.push_back(base_);                     // the state before the edit
    if (undo_.size() > kMaxDepth) undo_.erase(undo_.begin());
    redo_.clear();                              // a new edit ends the redo branch
    base_ = layout.elements;
}

bool EditHistory::undo(DayLayout& layout) {
    commit(layout, false);                      // make sure nothing is pending
    if (undo_.empty()) return false;
    redo_.push_back(base_);
    base_ = undo_.back();
    undo_.pop_back();
    layout.elements = base_;
    return true;
}

bool EditHistory::redo(DayLayout& layout) {
    commit(layout, false);
    if (redo_.empty()) return false;
    undo_.push_back(base_);
    base_ = redo_.back();
    redo_.pop_back();
    layout.elements = base_;
    return true;
}

}  // namespace interaction
