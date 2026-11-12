#include "calendar/calendar.h"
#include "calendar/day_storage.h"
#include "calendar/calendar_gl.h"
#include "animation/exporter.h"
#include "animation/timeline.h"
#include "polygons/polygons.h"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>

static int failures = 0;
#define CHECK(cond, name) do { \
    if (cond) std::cout << "PASS: " << name << "\n"; \
    else { std::cout << "FAIL: " << name << "\n"; failures++; } } while (0)

static bool near(double a, double b, double eps = 1e-4) { return std::fabs(a - b) < eps; }
static bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }
static bool sameRGBA(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }

static void testOption5StorageAndAnimations() {
    std::cout << "\n--- Option 5: Day Layout Saving, Thumbnails & Live Animations ---\n";

    // 1. All ShapeKind tokens round-trip and build valid polygons
    const ShapeKind kinds[] = {
        ShapeKind::STAR, ShapeKind::HEART, ShapeKind::BEZIER_HEART,
        ShapeKind::SQUARE, ShapeKind::HEXAGON, ShapeKind::CIRCLE,
        ShapeKind::ELLIPSE, ShapeKind::CLOUD, ShapeKind::CURVY_STAR,
        ShapeKind::RIBBON, ShapeKind::TAPE, ShapeKind::DATE_STAMP
    };
    bool shapesOk = true;
    for (ShapeKind k : kinds) {
        bool ok = false;
        std::string tok = shapeKindToString(k);
        ShapeKind back  = shapeKindFromString(tok, &ok);
        Polygon poly    = buildShapePolygon(k);
        if (!ok || back != k || poly.size() < 3 || polygonArea(poly) <= 1.0) {
            shapesOk = false;
        }
    }
    CHECK(shapesOk, "all 12 ShapeKind tokens round-trip and generate non-empty polygons");

    // 2. Ear-clipping triangulation preserves Shoelace polygonArea on concave shapes
    bool triAreaOk = true;
    for (ShapeKind k : {ShapeKind::STAR, ShapeKind::HEART, ShapeKind::CLOUD, ShapeKind::DATE_STAMP}) {
        Polygon p = buildShapePolygon(k);
        double expected = polygonArea(p);
        std::vector<Polygon> tris = triangulateConcave(p);
        double sum = 0.0;
        for (const auto& t : tris) sum += polygonArea(t);
        if (!near(sum, expected, expected * 0.01)) triAreaOk = false;
    }
    CHECK(triAreaOk, "ear-clipping triangulateConcave area matches Shoelace polygonArea");

    // 3. Save and load DayLayout round-trip to a text file (including sentence description)
    DayLayout orig = makeDefaultDayLayout(2026, 10, 14, "output/day_page.bmp");
    orig.photoFilter    = 2;
    orig.filterStrength = 0.75f;
    orig.description    = "SUNNY WALK AT THE PARK, COFFEE & CROISSANTS AT 10:30!";
    // Perturb an element to test custom transform + z-order persistence
    orig.elements[0].xform.position = {333.5, 412.25};
    orig.elements[0].xform.rotation = -27.5;
    orig.elements[0].xform.scale    = {1.35, 0.85};
    orig.elements[0].zOrder         = 99;
    orig.sortByZ();

    const std::string tmpFile = "output/test_day_2026_10_14.txt";
    CHECK(saveDayLayout(orig, tmpFile), "saveDayLayout writes text file");

    DayLayout loaded;
    CHECK(loadDayLayout(tmpFile, loaded), "loadDayLayout parses saved text file");
    CHECK(loaded.year == 2026 && loaded.month == 10 && loaded.day == 14, "loaded date matches");
    CHECK(loaded.photoPath == "output/day_page.bmp", "loaded photoPath matches");
    CHECK(loaded.description == orig.description, "loaded sentence description matches");
    CHECK(loaded.photoFilter == 2 && near(loaded.filterStrength, 0.75, 1e-3), "loaded filter matches");
    CHECK(loaded.elements.size() == orig.elements.size(), "loaded element count matches");

    bool elemsMatch = (loaded.elements.size() == orig.elements.size());
    for (size_t i = 0; elemsMatch && i < orig.elements.size(); ++i) {
        const auto& a = orig.elements[i];
        const auto& b = loaded.elements[i];
        if (a.kind != b.kind || a.zOrder != b.zOrder ||
            !near(a.xform.position.x, b.xform.position.x, 1e-2) ||
            !near(a.xform.position.y, b.xform.position.y, 1e-2) ||
            !near(a.xform.rotation,   b.xform.rotation,   1e-2) ||
            !near(a.xform.scale.x,    b.xform.scale.x,    1e-2) ||
            !near(a.xform.scale.y,    b.xform.scale.y,    1e-2) ||
            !sameRGBA(a.fill, b.fill) || !sameRGBA(a.edge, b.edge)) {
            elemsMatch = false;
        }
    }
    CHECK(elemsMatch, "every element's polygon type, transform, colour and z-order round-trips");
    std::remove(tmpFile.c_str());

    // Reject missing or malformed files cleanly
    DayLayout untouched = orig;
    CHECK(!loadDayLayout("output/no_such_day_file.txt", untouched), "missing layout file rejected");
    CHECK(!deserializeDayLayout("BAD_HEADER\n", untouched), "malformed header rejected");

    // 4. Thumbnail rendering reflects saved layout content & custom photo files
    Framebuffer emptyPhoto = resolveDayPhoto(makeDefaultDayLayout(2026, 10, 5), 60, 45);
    Framebuffer realPhoto  = resolveDayPhoto(orig, 60, 45);
    bool photosDistinct = false;
    for (size_t i = 0; i < emptyPhoto.pixels.size(); ++i) {
        if (!same(emptyPhoto.pixels[i], realPhoto.pixels[i])) { photosDistinct = true; break; }
    }
    CHECK(photosDistinct, "resolveDayPhoto distinguishes empty photo slot from actual picture file");

    Framebuffer thumbA = renderDayThumbnail(orig, 84, 44);
    CHECK(thumbA.width == 84 && thumbA.height == 44, "renderDayThumbnail dimensions 84x44");
    DayLayout modified = orig;
    SavedElement bigRed;
    bigRed.kind           = ShapeKind::SQUARE;
    bigRed.xform.position = {600, 300};
    bigRed.xform.scale    = {3.0, 3.0};
    bigRed.fill           = {250, 20, 20, 255};
    bigRed.zOrder         = 500;
    modified.elements.push_back(bigRed);
    Framebuffer thumbB = renderDayThumbnail(modified, 84, 44);
    bool thumbChanged = false;
    for (size_t i = 0; i < thumbA.pixels.size(); ++i) {
        if (!same(thumbA.pixels[i], thumbB.pixels[i])) { thumbChanged = true; break; }
    }
    CHECK(thumbChanged, "renderDayThumbnail updates when layout stickers change");

    // 5. Live drop-and-settle animation evaluator
    Transform target;
    target.position = {420.0, 280.0};
    target.rotation = 15.0;
    target.scale    = {1.2, 1.2};
    Transform atStart = evaluateDropAndSettle(target, 0.0, 0, 1);
    Transform atEnd   = evaluateDropAndSettle(target, 1.0, 0, 1);
    CHECK(atStart.position.y < target.position.y - 100.0 && atStart.scale.x < target.scale.x * 0.5,
          "drop-and-settle starts elevated and scaled down at t=0");
    CHECK(near(atEnd.position.x, target.position.x) &&
          near(atEnd.position.y, target.position.y) &&
          near(atEnd.rotation,   target.rotation) &&
          near(atEnd.scale.x,    target.scale.x),
          "drop-and-settle settles exactly on target Transform at t=1");

    // Verify overshoot from easeOutBack during drop
    double maxScaleX = 0.0;
    for (int i = 0; i <= 100; ++i) {
        Transform tr = evaluateDropAndSettle(target, i / 100.0, 0, 1);
        maxScaleX = std::max(maxScaleX, tr.scale.x);
    }
    CHECK(maxScaleX > target.scale.x * 1.02, "drop-and-settle overshoots scale via easeOutBack");

    // 6. Live page-turn clip region & flap
    Polygon clip0 = computePageTurnClipRegion(0.0, 800.0, 600.0);
    Polygon clipM = computePageTurnClipRegion(0.5, 800.0, 600.0);
    Polygon clip1 = computePageTurnClipRegion(1.0, 800.0, 600.0);
    CHECK(near(polygonArea(clip0), 800.0 * 600.0, 10.0), "page-turn region at t=0 covers full page");
    CHECK(polygonArea(clipM) > 50000.0 && polygonArea(clipM) < 430000.0,
          "page-turn region at t=0.5 covers partial page");
    CHECK(polygonArea(clip1) < 1.0, "page-turn region at t=1 uncovers new page completely");

    // 7. Month navigation layout calculation & persistence across app restarts
    CalendarLayout oct26 = makeMonthLayout(2026, 10);
    CalendarLayout feb24 = makeMonthLayout(2024, 2);
    CHECK(oct26.startCol == 4 && oct26.daysInMonth == 31, "Oct 2026 layout: startCol=4, 31 days");
    CHECK(feb24.daysInMonth == 29, "Feb 2024 leap year has 29 days");

    const std::string testSaveDir = "output/test_saves_opt5";
    std::error_code ec;
    std::filesystem::remove_all(testSaveDir, ec);

    // Create a sample user picture file to test uploading user's own photo
    const std::string userPicFile = "output/test_user_upload.bmp";
    {
        Framebuffer userPic(120, 90, {40, 160, 220, 255});
        userPic.saveBMP(userPicFile);
    }

    {
        CalendarScrapbookApp app;
        app.init(gl::Texture{}, gl::Texture{}, testSaveDir, "none");
        CHECK(app.viewMode() == CalendarScrapbookApp::VIEW_CALENDAR, "starts in calendar view");
        // Click day 5 cell to open its page
        Vec2 c5 = cellCentre(app.calendarLayout(), 5);
        app.onMouseButton(0, 1, c5.x, c5.y);
        CHECK(app.viewMode() == CalendarScrapbookApp::VIEW_DAY_PAGE && app.highlightedDay() == 5,
              "clicking highlighted day opens its day page");
        CHECK(!isCustomPhotoPath(app.currentDayLayout().photoPath),
              "new day starts with no photo until an actual picture is uploaded");

        size_t countBefore = app.currentDayLayout().elements.size();
        // Click slot 1 (HEART) in the bottom Sticker Palette (x=220, y=490) to add a heart sticker
        app.onMouseButton(0, 1, 220, 490);
        CHECK(app.currentDayLayout().elements.size() == countBefore + 1 &&
              app.currentDayLayout().elements.back().kind == ShapeKind::BEZIER_HEART,
              "clicking HEART slot in Sticker Palette adds a heart sticker to the page");

        // Click slot 0 (STAR) in the bottom Sticker Palette (x=140, y=490), then remove it via DEL STICKER button
        app.onMouseButton(0, 1, 140, 490);
        CHECK(app.currentDayLayout().elements.size() == countBefore + 2,
              "clicking STAR slot in Sticker Palette adds a star sticker");
        app.onMouseButton(0, 1, 610, 52); // Click "DEL STICKER" button in top toolbar
        CHECK(app.currentDayLayout().elements.size() == countBefore + 1,
              "clicking DEL STICKER removes the selected sticker");

        // Upload user's own photo file for Day 5
        CHECK(app.setCustomPhotoFromFile(userPicFile),
              "setCustomPhotoFromFile imports and saves user picture for Day 5");
        CHECK(isCustomPhotoPath(app.currentDayLayout().photoPath),
              "day 5 photoPath points to imported custom user picture");

        // Click inside the right-side Journal Text Area (x=600, y=200), clear it, and type a journal entry
        app.onMouseButton(0, 1, 705, 98); // Click "CLEAR" button in journal header
        CHECK(app.isEditingDescription() && app.currentDayLayout().description.empty(),
              "clicking CLEAR in journal header enters editing mode and clears journal");
        const std::string typedSentence = "Best trip to the beach!X";
        for (char c : typedSentence) {
            app.onChar((unsigned int)c);
        }
        app.onKey(259, 1); // Backspace removes trailing 'X'
        app.onKey(257, 1); // Enter finishes editing
        CHECK(!app.isEditingDescription() &&
              app.currentDayLayout().description == "BEST TRIP TO THE BEACH!",
              "typing in journal + Backspace + Enter updates and saves daily journal");
    }
    {
        // Reopen app and verify saved layout, custom photo, and journal text are restored
        CalendarScrapbookApp app2;
        app2.init(gl::Texture{}, gl::Texture{}, testSaveDir, "none");
        app2.openDayPage(5, false);
        CHECK(app2.currentDayLayout().elements.size() == 4,
              "reopening app restores saved layout with added sticker from disk");
        CHECK(isCustomPhotoPath(app2.currentDayLayout().photoPath),
              "reopening app restores user's custom uploaded photo path");
        CHECK(app2.currentDayLayout().description == "BEST TRIP TO THE BEACH!",
              "reopening app restores user's typed journal entry");
    }
    std::remove(userPicFile.c_str());
    std::filesystem::remove_all(testSaveDir, ec);
}

int main() {
    CalendarLayout L;

    // ---------- numeric checks ----------
    Vec2 c1 = cellCentre(L, 1);
    CHECK(near(c1.x, 493) && near(c1.y, 194), "day 1 centre == (493,194)");

    bool allOk = true;
    for (int d = 1; d <= L.daysInMonth; d++) {
        Vec2 c = cellCentre(L, d);
        if (dayAt(L, (int)c.x, (int)c.y) != d) allOk = false;
    }
    CHECK(allOk, "dayAt(cellCentre(d)) == d for all 31 days");
    CHECK(dayAt(L, 10, 10) == 0, "point outside the grid has no day");
    CHECK(dayAt(L, L.x0 + 10, L.y0 + 10) == 0, "empty cell before day 1 has no day");

    int from = 5, to = 19;
    Vec2 s0 = highlightCentre(L, from, to, 0.0), s1 = highlightCentre(L, from, to, 1.0);
    Vec2 a = cellCentre(L, from), b = cellCentre(L, to);
    CHECK(near(s0.x, a.x) && near(s0.y, a.y), "highlight starts exactly in the source cell");
    CHECK(near(s1.x, b.x) && near(s1.y, b.y), "highlight ends exactly in the target cell");
    CHECK(dayAt(L, (int)s1.x, (int)s1.y) == to, "last frame: dayAt(highlight) == target day");

    bool onScreen = true;
    for (int i = 0; i <= 100; i++) {
        Vec2 p = highlightCentre(L, from, to, i / 100.0);
        if (p.x < 0 || p.x > kFrameW || p.y < 0 || p.y > kFrameH) onScreen = false;
    }
    CHECK(onScreen, "highlight path stays on screen");

    CHECK(near(smoothstep01(0), 0) && near(smoothstep01(1), 1), "easing: f(0)=0 and f(1)=1");
    CHECK(near(pulseRadius(20, 0.0, 3), 20) && near(pulseRadius(20, 1.0, 3), 20),
          "pulse starts and ends at base radius (loops seamlessly)");
    CHECK(pulseRadius(20, 0.25 / 3, 3) > 20, "pulse grows after t = 0");

    // ---------- pixel checks ----------
    Framebuffer fb(kFrameW, kFrameH, {0, 0, 0});
    drawCalendarBackdrop(fb, L);
    int px = L.x0 + 3 * L.cw, py = L.y0 + L.ch + 10;           // a spot on a vertical grid line
    drawGridProgress(fb, L, 0.0);
    CHECK(!same(pixelAt(fb, px, py), {154, 106, 67}), "t=0: grid line not drawn yet");
    drawGridProgress(fb, L, 1.0);
    CHECK(same(pixelAt(fb, px, py), {154, 106, 67}), "t=1: grid line fully drawn");

    Framebuffer z(kFrameW, kFrameH, {0, 0, 0}), cal(kFrameW, kFrameH, {0, 0, 0}),
                day(kFrameW, kFrameH, {250, 244, 230});
    drawCalendarFull(cal, L);
    fillPolygon(day, makeStar({400, 260}, 120, 52), {228, 165, 69});
    drawZoomTransition(z, cal, day, L, 5, 1.0);
    bool identical = true;
    for (int y = 0; y < kFrameH && identical; y++)
        for (int x = 0; x < kFrameW; x++)
            if (!same(pixelAt(z, x, y), pixelAt(day, x, y))) { identical = false; break; }
    CHECK(identical, "zoom at t=1 equals the day page exactly");
    drawZoomTransition(z, cal, day, L, 5, 0.0);
    CHECK(same(pixelAt(z, 10, 10), pixelAt(cal, 10, 10)), "zoom at t=0 still shows the calendar");

    // ---------- frame sequences through Ian's exporter ----------
    struct Seq { const char* name; RenderFrameFn fn; int frames; };
    Seq seqs[] = {
        {"calendar_grid",   renderCalendarGrid,   12},
        {"calendar_select", renderCalendarSelect, 12},
        {"calendar_pulse",  renderCalendarPulse,  12},
        {"calendar_zoom",   renderCalendarZoom,   12},
    };
    ExportOptions expOpt;
    expOpt.dir = "output/test_cal_anim";
    for (const Seq& s : seqs) {
        Timeline tl; tl.frames = s.frames;
        ExportResult r = exportAnimation(s.name, tl, kFrameW, kFrameH, Color{154, 106, 67}, s.fn, expOpt);
        CHECK(r.ok && r.framesWritten == s.frames, std::string(s.name) + ": frames and contact sheet written");
    }
    std::error_code cleanEc;
    std::filesystem::remove_all(expOpt.dir, cleanEc);

    // ---------- Option 5 CP3 tests ----------
    testOption5StorageAndAnimations();

    std::cout << (failures == 0 ? "\nALL CHECKS PASSED\n" : "\nSOME CHECKS FAILED\n");
    return failures;
}
