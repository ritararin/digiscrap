// End-to-end tests for the interaction layer (CP3, Option 4): they drive the real
// CalendarScrapbookApp with scripted keys and mouse events (no window needed) and
// check duplicate, undo/redo, layer order and snapping the way a user would trigger them.
#include "calendar/calendar.h"
#include "calendar/calendar_gl.h"
#include "calendar/day_storage.h"
#include "polygons/polygons.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

static int failures = 0;
#define CHECK(cond, name) do { \
    if (cond) std::cout << "PASS: " << name << "\n"; \
    else { std::cout << "FAIL: " << name << "\n"; failures++; } } while (0)

static bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }

// GLFW key codes used below.
enum { KEY_C = 67, KEY_G = 71, KEY_N = 78, KEY_Q = 81, KEY_T = 84, KEY_Y = 89, KEY_Z = 90,
       KEY_D = 68, KEY_ENTER = 257, KEY_COMMA = 44, KEY_PERIOD = 46 };

static void press(CalendarScrapbookApp& a, int key)  { a.onKey(key, 1); }
static void click(CalendarScrapbookApp& a, double x, double y) { a.onMouseButton(0, 1, x, y); a.onMouseButton(0, 0, x, y); }
static void addSlot(CalendarScrapbookApp& a, int slot) { click(a, 146.0 + slot * 79.0, 495.0); }   // palette slots

static CalendarScrapbookApp* openDay5(CalendarScrapbookApp& app, const std::string& dir) {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    app.init(gl::Texture{}, gl::Texture{}, dir, "none");
    Vec2 c5 = cellCentre(app.calendarLayout(), 5);
    app.onMouseButton(0, 1, c5.x, c5.y);                       // open day 5
    return &app;
}

static const std::vector<SavedElement>& els(const CalendarScrapbookApp& a) { return a.currentDayLayout().elements; }
static const int kHex = 5;                                      // palette slot 5 = hexagon

static void testDuplicateUndoRedo() {
    std::cout << "\n--- Duplicate, undo and redo through the keyboard ---\n";
    CalendarScrapbookApp app;
    openDay5(app, "output/test_saves_opt4_a");
    CHECK(app.viewMode() == CalendarScrapbookApp::VIEW_DAY_PAGE, "day page is open");
    const size_t n0 = els(app).size();

    addSlot(app, kHex);
    CHECK(els(app).size() == n0 + 1 && app.selectedElementIndex() == (int)n0, "palette click adds and selects a sticker");
    const Vec2 p0 = els(app)[n0].xform.position;

    press(app, KEY_C);
    CHECK(els(app).size() == n0 + 2, "C duplicates the selected sticker");
    CHECK(app.selectedElementIndex() == (int)n0 + 1, "the copy becomes the selection");
    CHECK(els(app)[n0 + 1].kind == ShapeKind::HEXAGON && near(els(app)[n0 + 1].xform.position.x, p0.x + 24.0) &&
          near(els(app)[n0 + 1].xform.position.y, p0.y + 24.0), "the copy is the same shape, nudged by (24, 24)");
    CHECK(els(app)[n0 + 1].zOrder > els(app)[n0].zOrder, "the copy is on top");

    press(app, KEY_Z);
    CHECK(els(app).size() == n0 + 1, "Z undoes the duplicate");
    CHECK(app.selectedElementIndex() == -1, "undo clears the selection");
    press(app, KEY_Y);
    CHECK(els(app).size() == n0 + 2, "Y redoes it");
    press(app, KEY_Z); press(app, KEY_Z);
    CHECK(els(app).size() == n0, "two undos remove the duplicate and the sticker added before it");
    press(app, KEY_Z);
    CHECK(els(app).size() == n0, "undo with nothing left to undo does nothing (history starts when the day opens)");
    press(app, KEY_Y); press(app, KEY_Y);
    CHECK(els(app).size() == n0 + 2, "two redos bring both back");
    press(app, KEY_Y);
    CHECK(els(app).size() == n0 + 2, "redo with nothing left to redo does nothing");

    // The file on disk follows undo (so a reopened app agrees with what the user sees).
    press(app, KEY_Z);
    CalendarScrapbookApp app2;
    app2.init(gl::Texture{}, gl::Texture{}, "output/test_saves_opt4_a", "none");
    app2.openDayPage(5, false);
    CHECK(els(app2).size() == n0 + 1, "the saved layout matches the undone state");

    // Existing keys are untouched: D still replays the drop animation (no duplicate).
    const size_t before = els(app).size();
    press(app, KEY_D);
    CHECK(els(app).size() == before, "D keeps its old meaning (replay drop animation) and does not duplicate");
}

static void testDragIsOneStep() {
    std::cout << "\n--- A drag is one undo step ---\n";
    CalendarScrapbookApp app;
    openDay5(app, "output/test_saves_opt4_b");
    addSlot(app, kHex);
    const int idx = app.selectedElementIndex();
    const size_t n = els(app).size();
    const Vec2 start = els(app)[idx].xform.position;

    app.onMouseButton(0, 1, start.x, start.y);                  // press on the sticker
    for (int i = 1; i <= 30; i++) app.onCursorPos(start.x + 4.0 * i, start.y + 2.0 * i);
    app.onMouseButton(0, 0, start.x + 120.0, start.y + 60.0);   // release
    const int moved = app.selectedElementIndex();
    CHECK(moved >= 0 && std::fabs(els(app)[moved].xform.position.x - start.x) > 50.0, "the drag moved the sticker");

    press(app, KEY_Z);
    CHECK(els(app).size() == n, "one Z after a 30-move drag does not remove anything");
    bool back = false;
    for (const SavedElement& e : els(app))
        back = back || (e.kind == ShapeKind::HEXAGON && near(e.xform.position.x, start.x) && near(e.xform.position.y, start.y));
    CHECK(back, "one Z puts the sticker exactly where the drag began");
    press(app, KEY_Y);
    bool again = false;
    for (const SavedElement& e : els(app)) again = again || (e.kind == ShapeKind::HEXAGON && std::fabs(e.xform.position.x - start.x) > 50.0);
    CHECK(again, "Y replays the drag");

    // Other edits are undoable too: rotate button, right-click delete.
    addSlot(app, 0);
    const int sel = app.selectedElementIndex();
    const double rot0 = els(app)[sel].xform.rotation;
    click(app, 555, 454);                                       // ROT +
    CHECK(near(els(app)[sel].xform.rotation, rot0 + 15.0, 1e-4), "ROT + rotated the sticker");
    press(app, KEY_Z);
    CHECK(near(els(app)[sel].xform.rotation, rot0, 1e-4), "Z undoes the rotation");

    const size_t cnt = els(app).size();
    const Vec2 pos = els(app)[sel].xform.position;
    app.onMouseButton(1, 1, pos.x, pos.y);                      // right-click delete
    CHECK(els(app).size() == cnt - 1, "right-click deleted the sticker");
    press(app, KEY_Z);
    CHECK(els(app).size() == cnt, "Z brings a deleted sticker back");
}

static void testBackToBackDrags() {
    std::cout << "\n--- Two mouse drags in a row are two undo steps ---\n";
    CalendarScrapbookApp app;
    openDay5(app, "output/test_saves_opt4_f");
    addSlot(app, kHex);
    const Vec2 start = els(app)[app.selectedElementIndex()].xform.position;

    // Drag 1 and drag 2 with ONLY mouse events between them (no key press that could commit).
    app.onMouseButton(0, 1, start.x, start.y);
    app.onCursorPos(start.x + 100.0, start.y + 10.0);
    app.onMouseButton(0, 0, start.x + 100.0, start.y + 10.0);
    const Vec2 e1 = els(app)[app.selectedElementIndex()].xform.position;
    app.onMouseButton(0, 1, e1.x, e1.y);
    app.onCursorPos(e1.x - 60.0, e1.y + 50.0);
    app.onMouseButton(0, 0, e1.x - 60.0, e1.y + 50.0);

    auto hexAt = [&](Vec2 p) {
        for (const SavedElement& e : els(app))
            if (e.kind == ShapeKind::HEXAGON && near(e.xform.position.x, p.x) && near(e.xform.position.y, p.y)) return true;
        return false;
    };
    press(app, KEY_Z);
    CHECK(hexAt(e1) && !hexAt(start), "the first Z undoes only the second drag");
    press(app, KEY_Z);
    CHECK(hexAt(start) && !hexAt(e1), "the second Z undoes the first drag");
}

static void testLayers() {
    std::cout << "\n--- Layer order keys ---\n";
    CalendarScrapbookApp app;
    openDay5(app, "output/test_saves_opt4_c");
    addSlot(app, 0);                                            // star
    addSlot(app, 1);                                            // heart (selected, on top)
    const int top = app.selectedElementIndex();
    CHECK(top == (int)els(app).size() - 1 && els(app)[top].kind == ShapeKind::BEZIER_HEART, "heart starts on top");

    press(app, KEY_COMMA);
    CHECK(app.selectedElementIndex() == top - 1 && els(app)[top - 1].kind == ShapeKind::BEZIER_HEART,
          "comma moves the selected sticker one layer back (selection follows it)");
    CHECK(els(app)[top].kind == ShapeKind::STAR, "...and the sticker it passed is now above it");
    bool zOk = true;
    for (size_t i = 0; i < els(app).size(); i++) zOk = zOk && els(app)[i].zOrder == (int)i;
    CHECK(zOk, "zOrder matches the list order after the move");
    press(app, KEY_PERIOD);
    CHECK(app.selectedElementIndex() == top && els(app)[top].kind == ShapeKind::BEZIER_HEART, "period moves it forward again");
    press(app, KEY_PERIOD);
    CHECK(app.selectedElementIndex() == top, "already on top: period changes nothing");
    press(app, KEY_COMMA);
    press(app, KEY_Z);
    CHECK(app.selectedElementIndex() == -1 && els(app)[top].kind == ShapeKind::BEZIER_HEART, "a layer move can be undone");
}

static void testSnapping() {
    std::cout << "\n--- Snapping while dragging ---\n";
    CalendarScrapbookApp app;
    openDay5(app, "output/test_saves_opt4_d");
    while (!els(app).empty()) app.removeSelectedOrLastSticker();   // clean page: only A and B below
    addSlot(app, kHex);                                         // A
    addSlot(app, kHex);                                         // B (added last = on top)
    const Vec2 a = els(app)[0].xform.position;
    const int bIdx = app.selectedElementIndex();
    const Vec2 b = els(app)[bIdx].xform.position;

    auto lines = [](const SavedElement& e, double out[3], bool xAxis) {
        Polygon w = transformPolygon(buildShapePolygon(e.kind), e.xform.toMat3());
        Vec2 mn, mx;
        boundingBox(w, mn, mx);
        double lo = xAxis ? mn.x : mn.y, hi = xAxis ? mx.x : mx.y;
        out[0] = lo; out[1] = (lo + hi) / 2.0; out[2] = hi;
    };
    auto closestPair = [&](bool xAxis) {
        double la[3], lb[3];
        lines(els(app)[0], la, xAxis);
        lines(els(app)[app.selectedElementIndex()], lb, xAxis);
        double best = 1e9;
        for (double p : la) for (double q : lb) best = std::min(best, std::fabs(p - q));
        return best;
    };

    const Vec2 want{a.x + 3.0, 400.0};                          // 3 px off A's centre line, far below it
    auto dragBTo = [&](Vec2 to, Vec2 from) {
        app.onMouseButton(0, 1, from.x, from.y);
        app.onCursorPos(to.x - 20.0, to.y - 20.0);
        app.onCursorPos(to.x, to.y);
        app.onMouseButton(0, 0, to.x, to.y);
    };

    CHECK(app.snappingEnabled(), "snapping is on by default");
    dragBTo(want, b);
    CHECK(closestPair(true) < 1e-6, "dropped 3 px from A's centre line: B's lines snap onto A's");
    CHECK(near(els(app)[app.selectedElementIndex()].xform.position.y, 400.0), "the vertical position is untouched when nothing is close");

    // Snapping off: the sticker goes exactly where the mouse is.
    press(app, KEY_G);
    CHECK(!app.snappingEnabled(), "G turns snapping off");
    app.onKey(KEY_G, 2);                                        // key repeat must not toggle again
    CHECK(!app.snappingEnabled(), "holding G does not flicker the setting");
    const Vec2 cur = els(app)[app.selectedElementIndex()].xform.position;
    dragBTo(Vec2{a.x + 3.0, 400.0}, cur);
    CHECK(near(els(app)[app.selectedElementIndex()].xform.position.x, a.x + 3.0, 1e-9) && closestPair(true) > 1e-3,
          "with snapping off the sticker lands exactly under the mouse");
    press(app, KEY_G);
    CHECK(app.snappingEnabled(), "G turns snapping back on");

    // A sticker can never snap outside the page area.
    const Vec2 cur2 = els(app)[app.selectedElementIndex()].xform.position;
    dragBTo(Vec2{900.0, 700.0}, cur2);
    const Vec2 f = els(app)[app.selectedElementIndex()].xform.position;
    CHECK(f.x <= 755.0 + 1e-9 && f.y <= 525.0 + 1e-9, "snapping keeps the sticker inside the page area");
}

static void testEditingModeAndDays() {
    std::cout << "\n--- Shortcuts stay out of the way ---\n";
    CalendarScrapbookApp app;
    openDay5(app, "output/test_saves_opt4_e");
    addSlot(app, kHex);
    const size_t n = els(app).size();

    press(app, KEY_T);                                          // start typing a journal entry
    CHECK(app.isEditingDescription(), "T starts journal editing");
    press(app, KEY_C); press(app, KEY_Z); press(app, KEY_Y); press(app, KEY_G);
    CHECK(els(app).size() == n && app.snappingEnabled(), "while typing, C / Z / Y / G do not trigger shortcuts");
    press(app, KEY_ENTER);
    CHECK(!app.isEditingDescription(), "Enter finishes editing");

    // Another day starts with its own empty history: Z must not touch it.
    press(app, KEY_N);
    CHECK(app.highlightedDay() == 6, "N turns to the next day");
    const size_t day6 = els(app).size();
    press(app, KEY_Z);
    CHECK(els(app).size() == day6, "undo on a freshly opened day does nothing");

    // On the calendar view the shortcuts are inert.
    app.returnToCalendar(false);
    press(app, KEY_C); press(app, KEY_Z);
    CHECK(app.viewMode() == CalendarScrapbookApp::VIEW_CALENDAR, "shortcuts do nothing on the calendar view");
}

int main() {
    testDuplicateUndoRedo();
    testDragIsOneStep();
    testBackToBackDrags();
    testLayers();
    testSnapping();
    testEditingModeAndDays();
    std::error_code ec;
    for (const char* d : {"a", "b", "c", "d", "e", "f"})
        std::filesystem::remove_all(std::string("output/test_saves_opt4_") + d, ec);
    std::cout << (failures == 0 ? "\nALL INTERACTION APP CHECKS PASSED\n" : "\nSOME INTERACTION APP CHECKS FAILED\n");
    return failures == 0 ? 0 : 1;
}
