#pragma once
// Undo / redo for the stickers on a day page (CP3, Option 4: Interaction and Input).
//
// The history stores whole snapshots of a day's element list. A drag is made of
// hundreds of tiny mouse moves, but the user thinks of it as ONE edit, so the
// history is only updated when an action FINISHES, never while a drag is running.
#include <cstddef>
#include <vector>
#include "calendar/day_storage.h"   // DayLayout, SavedElement

namespace interaction {

// True when two element lists look identical: same stickers, in the same drawing order,
// with the same shape, transform and colours. (Raw zOrder numbers are not compared.)
bool sameElements(const std::vector<SavedElement>& a, const std::vector<SavedElement>& b);

class EditHistory {
public:
    static constexpr std::size_t kMaxDepth = 100;

    // Compare `layout` with the state seen at the previous commit:
    //  - first time, or a different day  -> start a fresh history for that day
    //  - gestureInProgress (mouse held)  -> do nothing yet
    //  - elements changed                -> the old state becomes one undo step
    void commit(const DayLayout& layout, bool gestureInProgress);

    // Restore the previous / next state into layout.elements. False if there is none.
    bool undo(DayLayout& layout);
    bool redo(DayLayout& layout);

    bool        canUndo() const   { return !undo_.empty(); }
    bool        canRedo() const   { return !redo_.empty(); }
    std::size_t undoDepth() const { return undo_.size(); }
    std::size_t redoDepth() const { return redo_.size(); }

private:
    struct DayKey {
        int year = 0, month = 0, day = 0;
        bool operator==(const DayKey& o) const { return year == o.year && month == o.month && day == o.day; }
    };

    bool                                 hasBase_ = false;
    DayKey                               key_;
    std::vector<SavedElement>            base_;    // the current state, as of the last commit
    std::vector<std::vector<SavedElement>> undo_;
    std::vector<std::vector<SavedElement>> redo_;
};

// RAII helper: put one on the first line of any function that can edit stickers.
// It commits when the function starts (so the state BEFORE the edit is known)
// and again when the function returns by ANY path (so the edit becomes an undo step).
class EditScope {
public:
    EditScope(EditHistory& h, const DayLayout& layout, const bool& gestureInProgress)
        : h_(h), layout_(layout), busy_(gestureInProgress) { h_.commit(layout_, busy_); }
    ~EditScope() { h_.commit(layout_, busy_); }

    EditScope(const EditScope&)            = delete;
    EditScope& operator=(const EditScope&) = delete;

private:
    EditHistory&     h_;
    const DayLayout& layout_;
    const bool&      busy_;
};

}  // namespace interaction
