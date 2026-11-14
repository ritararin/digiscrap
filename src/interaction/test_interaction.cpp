// Tests for the interaction layer logic (CP3, Option 4): undo/redo history,
// duplicate and layer ordering, snapping and the keyboard shortcut table.
// Pure logic: no window, no OpenGL.   Run with:  make test
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "calendar/day_storage.h"
#include "polygons/polygons.h"
#include "interaction/interaction.h"

static int failures = 0;
#define CHECK(cond, name) do { \
    if (cond) std::cout << "PASS: " << name << "\n"; \
    else { std::cout << "FAIL: " << name << "\n"; failures++; } } while (0)

static bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) < eps; }

static SavedElement makeEl(ShapeKind k, double x, double y, int z, double rot = 0.0, double sc = 1.0) {
    SavedElement e;
    e.kind = k;
    e.xform.position = {x, y};
    e.xform.rotation = rot;
    e.xform.scale = {sc, sc};
    e.zOrder = z;
    return e;
}

// World bounding box of an element, computed independently of snapping.cpp.
struct WBox { double x0, y0, x1, y1; };
static WBox boxOf(const SavedElement& e, Vec2 pos) {
    Transform xf = e.xform;
    xf.position = pos;
    Vec2 mn, mx;
    boundingBox(transformPolygon(buildShapePolygon(e.kind), xf.toMat3()), mn, mx);
    return {mn.x, mn.y, mx.x, mx.y};
}

// ---------------------------------------------------------------------------
static void testHistory() {
    std::cout << "\n--- Undo / redo history ---\n";
    using namespace interaction;

    DayLayout day;
    EditHistory h;
    h.commit(day, false);                                   // empty page: baseline
    CHECK(!h.canUndo() && !h.canRedo(), "fresh history has nothing to undo or redo");
    CHECK(!h.undo(day) && !h.redo(day), "undo/redo on an empty history return false");

    day.elements.push_back(makeEl(ShapeKind::STAR, 200, 200, 0));
    h.commit(day, false);
    CHECK(h.undoDepth() == 1, "adding a sticker is one undo step");
    h.commit(day, false);
    CHECK(h.undoDepth() == 1, "committing an unchanged page adds no step");

    // Re-numbering zOrder without changing the drawing order is not an edit
    // (clicking a sticker that is already on top bumps its zOrder).
    day.elements[0].zOrder += 7;
    h.commit(day, false);
    CHECK(h.undoDepth() == 1, "a zOrder bump that does not change the drawing order is not an undo step");

    // A drag: many tiny changes while the mouse is held, then one step when it is released.
    const Vec2 before = day.elements[0].xform.position;
    for (int i = 1; i <= 50; i++) {
        day.elements[0].xform.position = {200.0 + i, 200.0 - i};
        h.commit(day, true);
    }
    CHECK(h.undoDepth() == 1, "no undo steps are recorded while the drag is still running");
    h.commit(day, false);
    CHECK(h.undoDepth() == 2, "the whole drag becomes ONE undo step on release");

    CHECK(h.undo(day), "undo succeeds");
    CHECK(day.elements.size() == 1 && near(day.elements[0].xform.position.x, before.x) &&
          near(day.elements[0].xform.position.y, before.y), "undo puts the sticker back where the drag began");
    CHECK(h.canRedo() && h.redoDepth() == 1, "undo makes redo available");
    CHECK(h.undo(day) && day.elements.empty(), "second undo removes the sticker again");
    CHECK(!h.undo(day), "nothing left to undo");
    CHECK(h.redo(day) && day.elements.size() == 1 && near(day.elements[0].xform.position.x, before.x),
          "redo brings the sticker back");
    CHECK(h.redo(day) && near(day.elements[0].xform.position.x, 250.0), "second redo restores the dragged position");
    CHECK(!h.redo(day), "nothing left to redo");

    // A new edit after an undo ends the redo branch.
    h.undo(day);
    day.elements[0].xform.rotation = 45.0;
    h.commit(day, false);
    CHECK(!h.canRedo(), "a new edit discards the redo steps");

    // Changing day starts a fresh history.
    day.day = 6;
    day.elements.clear();
    h.commit(day, false);
    CHECK(!h.canUndo() && !h.canRedo(), "opening another day starts a fresh history");

    // Depth limit: only the newest kMaxDepth steps are kept.
    DayLayout big;
    EditHistory hb;
    hb.commit(big, false);
    for (int i = 0; i < 150; i++) {
        big.elements.push_back(makeEl(ShapeKind::SQUARE, 100 + i, 100, i));
        hb.commit(big, false);
    }
    CHECK(hb.undoDepth() == EditHistory::kMaxDepth, "history keeps at most kMaxDepth steps");
    int undone = 0;
    while (hb.undo(big)) undone++;
    CHECK(undone == (int)EditHistory::kMaxDepth && big.elements.size() == 50,
          "the oldest steps are the ones dropped");

    // EditScope commits on every way out of a function.
    DayLayout sday;
    EditHistory sh;
    bool busy = false;
    auto addIfAsked = [&](bool really) {
        EditScope scope(sh, sday, busy);
        if (!really) return;                                // early return path
        sday.elements.push_back(makeEl(ShapeKind::HEART, 300, 300, 0));
    };
    addIfAsked(false);
    CHECK(sh.undoDepth() == 0, "EditScope: a call that changes nothing records nothing");
    addIfAsked(true);
    CHECK(sh.undoDepth() == 1 && sh.undo(sday) && sday.elements.empty(),
          "EditScope: the first edit is undoable (state before it was captured)");
    sh.redo(sday);
    busy = true;
    {
        EditScope scope(sh, sday, busy);
        sday.elements[0].xform.position = {1, 1};
    }
    CHECK(sh.undoDepth() == 1, "EditScope: no step while a gesture is in progress");
    busy = false;
    { EditScope scope(sh, sday, busy); }
    CHECK(sh.undoDepth() == 2, "EditScope: the step lands once the gesture ends");
}

// ---------------------------------------------------------------------------
static void testLayerOps() {
    std::cout << "\n--- Duplicate and layer order ---\n";
    using namespace interaction;
    const PageArea area;

    std::vector<SavedElement> v = {
        makeEl(ShapeKind::STAR,   200, 200, 0, 15.0, 1.2),
        makeEl(ShapeKind::HEART,  300, 220, 1),
        makeEl(ShapeKind::SQUARE, 400, 240, 2),
        makeEl(ShapeKind::CIRCLE, 500, 260, 3),
    };

    int idx = duplicateElement(v, 0, area);
    CHECK(idx == 4 && v.size() == 5, "duplicate adds one sticker and returns its index");
    CHECK(v[4].kind == ShapeKind::STAR && near(v[4].xform.rotation, 15.0) && near(v[4].xform.scale.x, 1.2),
          "the copy keeps shape, rotation and scale");
    CHECK(near(v[4].xform.position.x, 224.0) && near(v[4].xform.position.y, 224.0), "the copy is nudged by (24, 24)");
    CHECK(v[4].zOrder == 4, "the copy goes on top");
    CHECK(near(v[0].xform.position.x, 200.0) && v[0].zOrder == 0, "the original is untouched");

    std::vector<SavedElement> corner = {makeEl(ShapeKind::SQUARE, 750, 520, 0)};
    duplicateElement(corner, 0, area);
    CHECK(near(corner[1].xform.position.x, area.x1) && near(corner[1].xform.position.y, area.y1),
          "a copy made near the edge stays inside the page area");

    CHECK(duplicateElement(v, -1, area) == -1 && duplicateElement(v, 5, area) == -1 && v.size() == 5,
          "duplicate with a bad index does nothing");

    // Layers.
    std::vector<SavedElement> L = {
        makeEl(ShapeKind::STAR, 100, 100, 0), makeEl(ShapeKind::HEART, 110, 100, 1),
        makeEl(ShapeKind::SQUARE, 120, 100, 2), makeEl(ShapeKind::CIRCLE, 130, 100, 3)};

    int n = moveLayer(L, 1, +1);
    CHECK(n == 2 && L[2].kind == ShapeKind::HEART && L[1].kind == ShapeKind::SQUARE, "one layer forward swaps with the sticker above");
    bool zOk = true;
    for (int i = 0; i < (int)L.size(); i++) zOk = zOk && L[i].zOrder == i;
    CHECK(zOk, "zOrder always matches the list order");
    {
        interaction::EditHistory eh;
        DayLayout dl; dl.elements = L;
        eh.commit(dl, false);
        interaction::moveLayer(dl.elements, 0, +1);
        eh.commit(dl, false);
        CHECK(eh.undoDepth() == 1, "changing the drawing order IS an undo step");
    }
    CHECK(near(L[2].xform.position.x, 110) && L[2].kind == ShapeKind::HEART, "the moved sticker keeps its position");

    n = moveLayer(L, 2, -1);
    CHECK(n == 1 && L[1].kind == ShapeKind::HEART, "one layer back undoes the move");
    CHECK(moveLayer(L, 3, +1) == 3 && L[3].kind == ShapeKind::CIRCLE, "the top sticker cannot go further forward");
    CHECK(moveLayer(L, 0, -1) == 0 && L[0].kind == ShapeKind::STAR, "the bottom sticker cannot go further back");
    CHECK(moveLayer(L, 0, +99) == 3 && L[3].kind == ShapeKind::STAR, "a big step goes straight to the front");
    CHECK(moveLayer(L, 3, -99) == 0 && L[0].kind == ShapeKind::STAR, "a big negative step goes straight to the back");
    CHECK(moveLayer(L, -1, 1) == -1 && moveLayer(L, 9, 1) == -1, "moveLayer with a bad index returns -1");

    std::vector<SavedElement> ties = {
        makeEl(ShapeKind::STAR, 0, 0, 5), makeEl(ShapeKind::HEART, 0, 0, 5), makeEl(ShapeKind::SQUARE, 0, 0, 5)};
    moveLayer(ties, 0, +1);
    CHECK(ties[0].zOrder == 0 && ties[1].zOrder == 1 && ties[2].zOrder == 2 && ties[1].kind == ShapeKind::STAR,
          "stickers sharing a zOrder get a clear order after a move");
}

// ---------------------------------------------------------------------------
static void testSnapping() {
    std::cout << "\n--- Snapping ---\n";
    using namespace interaction;
    const PageArea area;

    // Fixed square A, moving square B.
    std::vector<SavedElement> v = {
        makeEl(ShapeKind::SQUARE, 300, 300, 0),
        makeEl(ShapeKind::SQUARE, 500, 400, 1),
    };
    const WBox A = boxOf(v[0], v[0].xform.position);
    const double halfW = (boxOf(v[1], {0, 0}).x1 - boxOf(v[1], {0, 0}).x0) / 2.0;
    const double halfH = (boxOf(v[1], {0, 0}).y1 - boxOf(v[1], {0, 0}).y0) / 2.0;

    // B's left edge 3 px to the right of A's right edge -> snaps flush against A.
    Vec2 desired{A.x1 + halfW + 3.0, 420.0};
    SnapResult r = snapElement(v, 1, desired, area);
    CHECK(r.guides.hasX && near(r.guides.x, A.x1, 1e-6), "left edge snaps to the neighbour's right edge (guide at that edge)");
    CHECK(near(boxOf(v[1], r.position).x0, A.x1, 1e-6), "after the snap the two edges coincide exactly");
    CHECK(near(r.position.y, 420.0), "the other axis is untouched when nothing is close");

    // Just outside the threshold: no snap.
    desired = {A.x1 + halfW + kDefaultSnapThreshold + 1.0, 420.0};
    r = snapElement(v, 1, desired, area);
    CHECK(!r.guides.hasX && near(r.position.x, desired.x), "7 px away: no snap");
    // Exactly at the threshold: snap.
    desired = {A.x1 + halfW + kDefaultSnapThreshold, 420.0};
    r = snapElement(v, 1, desired, area);
    CHECK(r.guides.hasX, "exactly at the threshold: snaps");

    // Centre lines: 4 px above A's centre line.
    desired = {460.0, (A.y0 + A.y1) / 2.0 - 4.0};
    r = snapElement(v, 1, desired, area);
    CHECK(r.guides.hasY && near(r.position.y, (A.y0 + A.y1) / 2.0, 1e-6), "centre line snaps to the neighbour's centre line");

    // Both axes at once.
    desired = {A.x1 + halfW + 2.0, A.y0 + halfH - 3.0};
    r = snapElement(v, 1, desired, area);
    CHECK(r.guides.hasX && r.guides.hasY, "both axes can snap together");

    // Several candidates: the closest wins.
    std::vector<SavedElement> two = {
        makeEl(ShapeKind::SQUARE, 300, 300, 0), makeEl(ShapeKind::SQUARE, 306, 500, 1), makeEl(ShapeKind::SQUARE, 600, 600, 2)};
    r = snapElement(two, 2, {301.0, 600.0}, area);
    CHECK(near(r.position.x, 300.0, 1e-6) && near(r.guides.x, 300.0, 1e-6), "the nearest of several centre lines wins");

    // Page area: a lone sticker snaps to the area's centre and edges.
    std::vector<SavedElement> solo = {makeEl(ShapeKind::SQUARE, 0, 0, 0)};
    r = snapElement(solo, 0, {area.centre().x + 3.0, area.centre().y - 2.0}, area);
    CHECK(near(r.position.x, area.centre().x) && near(r.position.y, area.centre().y) && r.guides.hasX && r.guides.hasY,
          "a lone sticker snaps to the centre of the page area");
    const double soloHalf = (boxOf(solo[0], {0, 0}).x1 - boxOf(solo[0], {0, 0}).x0) / 2.0;
    r = snapElement(solo, 0, {area.x0 + soloHalf + 4.0, 150.0}, area);
    CHECK(near(boxOf(solo[0], r.position).x0, area.x0, 1e-6), "a sticker near the left margin snaps to it");

    // Never snaps to itself; bad index and empty list are safe.
    r = snapElement(solo, 0, {150.0, 150.0}, area);
    CHECK(!r.guides.hasX && !r.guides.hasY, "far from everything: no guides");
    r = snapElement(solo, 5, {123.0, 456.0}, area);
    CHECK(near(r.position.x, 123.0) && near(r.position.y, 456.0), "a bad index returns the desired position");
    r = snapElement({}, 0, {1.0, 2.0}, area);
    CHECK(near(r.position.x, 1.0), "an empty list is safe");

    // Rotation is taken into account (the bounding box of the rotated shape is used).
    std::vector<SavedElement> rot = {makeEl(ShapeKind::SQUARE, 300, 300, 0), makeEl(ShapeKind::SQUARE, 500, 400, 1, 45.0)};
    const double rotHalf = (boxOf(rot[1], {0, 0}).x1 - boxOf(rot[1], {0, 0}).x0) / 2.0;
    r = snapElement(rot, 1, {A.x1 + rotHalf + 2.0, 450.0}, area);
    CHECK(near(boxOf(rot[1], r.position).x0, A.x1, 1e-6), "a rotated sticker snaps by its rotated bounding box");
}

// ---------------------------------------------------------------------------
static void testShortcuts() {
    std::cout << "\n--- Keyboard shortcuts and help line ---\n";
    using namespace interaction;

    CHECK(actionForKey(67) == Action::Duplicate && actionForKey(90) == Action::Undo &&
          actionForKey(89) == Action::Redo && actionForKey(71) == Action::ToggleSnap &&
          actionForKey(72) == Action::ToggleHelp, "C / Z / Y / G / H map to copy / undo / redo / snap / help");
    CHECK(actionForKey(46) == Action::LayerUp && actionForKey(93) == Action::LayerUp &&
          actionForKey(44) == Action::LayerDown && actionForKey(91) == Action::LayerDown,
          "period and ] move up, comma and [ move down");

    // Keys the day page already uses must stay untouched.
    const int reserved[] = {
        66 /*B*/, 258 /*Tab*/, 84 /*T*/, 257 /*Enter*/, 335 /*KP Enter*/, 85 /*U*/, 78 /*N*/, 80 /*P*/,
        68 /*D*/, 83 /*S*/, 76 /*L*/, 70 /*F*/, 65 /*A*/, 261 /*Delete*/, 259 /*Backspace*/, 88 /*X*/,
        81 /*Q*/, 69 /*E*/, 262, 263, 264, 265 /*arrows*/, 61 /*=*/, 334 /*KP+*/, 45 /*-*/, 333 /*KP-*/};
    bool clash = false;
    for (int k : reserved) clash = clash || actionForKey(k) != Action::None;
    CHECK(!clash, "no new shortcut collides with a key the day page already uses");

    // Help text: drawable characters only, and it fits the ribbon.
    for (bool snap : {true, false}) {
        const std::string s = shortcutLine(snap);
        bool drawable = true;
        for (char c : s) {
            bool ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || std::strchr(" -.,!?':/", c) != nullptr;
            drawable = drawable && ok;
        }
        CHECK(drawable, std::string("help line uses only characters the bitmap font can draw (snap ") + (snap ? "on" : "off") + ")");
        CHECK((int)s.size() * 8 - 2 <= 682, std::string("help line fits the 682 px status ribbon (snap ") + (snap ? "on" : "off") + ")");
    }
    CHECK(shortcutLine(true).find("SNAP ON") != std::string::npos && shortcutLine(false).find("SNAP OFF") != std::string::npos,
          "help line shows the snapping state");
}

int main() {
    testHistory();
    testLayerOps();
    testSnapping();
    testShortcuts();
    std::cout << (failures == 0 ? "\nALL INTERACTION CHECKS PASSED\n" : "\nSOME INTERACTION CHECKS FAILED\n");
    return failures == 0 ? 0 : 1;
}
