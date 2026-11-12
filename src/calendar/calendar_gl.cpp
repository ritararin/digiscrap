#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "calendar/calendar_gl.h"
#include "opengl/renderer.h"
#include "easing/easing.h"
#include "paths/paths.h"
#include "polygons/polygons.h"
#include "curves/curves.h"
#include "animation/timeline.h"
#include "animation/lerp.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

const Color kLeather   {102,  68,  44, 255};
const Color kBrown     {154, 106,  67, 255};
const Color kDark      { 64,  58,  48, 255};
const Color kCream     {250, 236, 208, 255};
const Color kPaper     {253, 245, 226, 255};
const Color kWeekend   {246, 224, 184, 255};
const Color kWhite     {255, 252, 245, 255};
const Color kMustard   {228, 165,  69, 255};
const Color kHoney     {249, 205, 125, 255};
const Color kGreen     { 96, 134, 108, 255};
const Color kDeepGreen { 68,  98,  78, 255};
const Color kSage      {173, 170, 134, 255};
const Color kTerracotta{196,  92,  70, 255};
const Color kShadow    {208, 180, 136, 255};

Polygon rectPoly(double x0, double y0, double x1, double y1) {
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
}

double cross2D(Vec2 a, Vec2 b, Vec2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool pointInTri(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    double c1 = cross2D(a, b, p);
    double c2 = cross2D(b, c, p);
    double c3 = cross2D(c, a, p);
    bool hasNeg = (c1 < -1e-9) || (c2 < -1e-9) || (c3 < -1e-9);
    bool hasPos = (c1 >  1e-9) || (c2 >  1e-9) || (c3 >  1e-9);
    return !(hasNeg && hasPos);
}

void drawConcavePolygonGL(const Polygon& poly, const Mat3& transform, Color fill) {
    if (poly.size() < 3) return;
    if (poly.size() <= 4) {
        gl::drawPolygon(poly, transform, fill);
        return;
    }
    std::vector<Polygon> tris = triangulateConcave(poly);
    for (const auto& tri : tris) {
        gl::drawPolygon(tri, transform, fill);
    }
}

void drawRectGL(double x0, double y0, double x1, double y1, Color fill, Color edge = {0, 0, 0, 0}) {
    Polygon r = rectPoly(x0, y0, x1, y1);
    gl::drawPolygon(r, Mat3::identity(), fill);
    if (edge.a > 0) {
        gl::drawPolyline(r, Mat3::identity(), edge);
    }
}

bool insideRect(double x, double y, double x0, double y0, double x1, double y1) {
    return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}

// Compute exact pixel width of a string rendered with the 3x5 bitmap font at integer `scale`.
int crispTextWidth(const std::string& s, int scale) {
    if (s.empty() || scale <= 0) return 0;
    return (int)s.size() * 4 * scale - scale;
}

// Draw razor-sharp integer-scaled bitmap text snapped to whole screen pixels,
// with an optional 1px contrast shadow for readability.
void drawCrispText(const gl::Texture& fontTex, const std::string& text,
                   double x, double y, int scale, Color col,
                   bool withShadow = false, Color shadowCol = {255, 252, 245, 180}) {
    if (text.empty() || scale <= 0) return;
    float ix = std::round((float)x);
    float iy = std::round((float)y);
    float sc = (float)scale;
    if (withShadow) {
        gl::drawStampText(fontTex, text, ix + 1.0f, iy + 1.0f, sc, shadowCol);
    }
    gl::drawStampText(fontTex, text, ix, iy, sc, col);
}

void drawCrispTextCentered(const gl::Texture& fontTex, const std::string& text,
                           double centerX, double centerY, int scale, Color col,
                           bool withShadow = false, Color shadowCol = {255, 252, 245, 180}) {
    int w = crispTextWidth(text, scale);
    int h = 5 * scale;
    drawCrispText(fontTex, text, centerX - w * 0.5, centerY - h * 0.5,
                  scale, col, withShadow, shadowCol);
}

void drawButtonGL(const gl::Texture& fontTex, double x0, double y0, double x1, double y1,
                  const std::string& label, Color bg, Color fg, bool hovered = false) {
    Polygon r = rectPoly(x0, y0, x1, y1);
    gl::drawPolygon(r, Mat3::translate(2, 3), Color{64, 58, 48, 85});
    Color fillCol = hovered ? lerp(bg, kWhite, 0.28) : bg;
    gl::drawPolygon(r, Mat3::identity(), fillCol);
    gl::drawPolyline(r, Mat3::identity(), hovered ? kTerracotta : kBrown);
    // Subtle top highlight line inside button
    gl::drawPolygon(rectPoly(x0 + 2, y0 + 2, x1 - 2, y0 + 4), Mat3::identity(),
                    Color{255, 255, 255, 70});
    drawCrispTextCentered(fontTex, label, (x0 + x1) * 0.5, (y0 + y1) * 0.5, 2, fg);
}

double hash01(int n) {
    unsigned x = (unsigned)n * 2654435761u;
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;
    return (x & 0xFFFFFFu) / 16777216.0;
}

const char* shortFilterLabel(int kind) {
    switch (kind) {
        case 1: return "GRAY";
        case 2: return "SEPIA";
        case 3: return "TINT";
        case 4: return "BRIGHT";
        case 5: return "CONTR";
        case 6: return "INVERT";
        default: return "NORMAL";
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Ear-clipping triangulation for concave polygons
// ---------------------------------------------------------------------------

std::vector<Polygon> triangulateConcave(const Polygon& poly) {
    std::vector<Polygon> tris;
    if (poly.size() < 3) return tris;
    if (poly.size() == 3) {
        tris.push_back(poly);
        return tris;
    }

    std::vector<Vec2> pts;
    pts.reserve(poly.size());
    for (size_t i = 0; i < poly.size(); ++i) {
        Vec2 next = poly[(i + 1) % poly.size()];
        if (std::hypot(poly[i].x - next.x, poly[i].y - next.y) > 1e-6) {
            pts.push_back(poly[i]);
        }
    }
    if (pts.size() < 3) return tris;

    double signedArea2 = 0.0;
    for (size_t i = 0; i < pts.size(); ++i) {
        Vec2 a = pts[i], b = pts[(i + 1) % pts.size()];
        signedArea2 += a.x * b.y - b.x * a.y;
    }
    const double orient = (signedArea2 >= 0.0) ? 1.0 : -1.0;

    std::vector<int> idx(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) idx[i] = (int)i;

    int guard = (int)pts.size() * 4;
    while (idx.size() > 3 && guard-- > 0) {
        bool earFound = false;
        const int m = (int)idx.size();
        for (int i = 0; i < m; ++i) {
            int iPrev = idx[(i + m - 1) % m];
            int iCurr = idx[i];
            int iNext = idx[(i + 1) % m];

            Vec2 a = pts[iPrev], b = pts[iCurr], c = pts[iNext];
            if (cross2D(a, b, c) * orient <= 1e-7) continue;

            bool hasPointInside = false;
            for (int j = 0; j < m; ++j) {
                int k = idx[j];
                if (k == iPrev || k == iCurr || k == iNext) continue;
                if (pointInTri(pts[k], a, b, c)) {
                    hasPointInside = true;
                    break;
                }
            }
            if (!hasPointInside) {
                tris.push_back({a, b, c});
                idx.erase(idx.begin() + i);
                earFound = true;
                break;
            }
        }
        if (!earFound) break;
    }

    if (idx.size() == 3) {
        tris.push_back({pts[idx[0]], pts[idx[1]], pts[idx[2]]});
    } else if (idx.size() > 3) {
        for (size_t i = 1; i + 1 < idx.size(); ++i) {
            tris.push_back({pts[idx[0]], pts[idx[i]], pts[idx[i + 1]]});
        }
    }
    return tris;
}

// ---------------------------------------------------------------------------
// Pure animation evaluators (CP2 easing + motion paths -> CP3 live OpenGL)
// ---------------------------------------------------------------------------

Transform evaluateDropAndSettle(const Transform& target, double t,
                                int index, int totalCount) {
    t = easing::clamp01(t);
    if (t >= 1.0) return target;

    int count = std::max(1, totalCount);
    double maxDelay = (count > 1) ? 0.36 : 0.0;
    double start = (count > 1) ? (maxDelay * (double)index / (double)(count - 1)) : 0.0;
    double lt = segmentT(t, start, 1.0);

    double bounceP = easing::easeOutBounce(lt);
    double backP   = easing::easeOutBack(lt);
    double cubicP  = easing::easeOutCubic(lt);

    Transform out = target;
    out.position.x = lerp(target.position.x - 36.0, target.position.x, cubicP);
    out.position.y = lerp(target.position.y - 220.0, target.position.y, bounceP);

    double scaleFactor = 0.15 + 0.85 * backP;
    out.scale = {target.scale.x * scaleFactor, target.scale.y * scaleFactor};
    out.rotation = lerp(target.rotation - 32.0, target.rotation, cubicP);
    return out;
}

Polygon computePageTurnClipRegion(double t, double width, double height) {
    double eased = easing::easeInOutCubic(easing::clamp01(t));
    std::vector<motion::Point2> region = motion::pageCurlRegion(eased, width, height, 28);
    Polygon out;
    out.reserve(region.size());
    for (const auto& p : region) {
        out.push_back({p.x, p.y});
    }
    return out;
}

Polygon computePageTurnFlap(double t, double width, double height) {
    double eased = easing::easeInOutCubic(easing::clamp01(t));
    std::vector<motion::Point2> edge = motion::pageCurlEdge(eased, width, height, 24);
    double flapW = 55.0 * std::sin(M_PI * eased);

    Polygon flap;
    flap.reserve(edge.size() * 2);
    for (const auto& p : edge) {
        flap.push_back({p.x, p.y});
    }
    for (int i = (int)edge.size() - 1; i >= 0; --i) {
        double ratio = (height > 0.0) ? (edge[i].y / height) : 0.5;
        double arch = 0.65 + 0.35 * std::sin(M_PI * ratio);
        flap.push_back({std::min(width, edge[i].x + flapW * arch), edge[i].y});
    }
    return flap;
}

// ---------------------------------------------------------------------------
// CalendarScrapbookApp implementation
// ---------------------------------------------------------------------------

CalendarScrapbookApp::CalendarScrapbookApp() = default;

CalendarScrapbookApp::~CalendarScrapbookApp() {
    clearMonthThumbnails();
}

void CalendarScrapbookApp::init(const gl::Texture& photoTex, const gl::Texture& fontTex,
                                const std::string& saveDir, const std::string& defaultPhotoPath) {
    photoTex_         = photoTex;
    fontTex_          = fontTex;
    saveDir_          = saveDir.empty() ? "saves" : saveDir;
    defaultPhotoPath_ = defaultPhotoPath.empty() ? "demo" : defaultPhotoPath;

    calLayout_      = makeMonthLayout(2026, 10);
    highlightedDay_ = 5;
    view_           = VIEW_CALENDAR;

    // Generate all 8 distinct daily theme textures on the GPU when an OpenGL context is active
    if (fontTex_.id != 0 || photoTex_.id != 0) {
        for (int i = 0; i < kNumPhotoThemes; ++i) {
            Framebuffer img = makeDayPhoto(i, 360, 270);
            themePhotos_[i] = gl::createTexture(img, false);
        }
    }

    // Seed saved layouts across the month so the calendar showcases rich daily photo thumbnails
    for (int d : {1, 2, 3, 4, 5, 8, 14, 22}) {
        if (!hasSavedDayLayout(saveDir_, calLayout_.year, calLayout_.month, d)) {
            DayLayout seeded = makeDefaultDayLayout(calLayout_.year, calLayout_.month, d, defaultPhotoPath_);
            saveDayLayout(seeded, dayLayoutPath(saveDir_, calLayout_.year, calLayout_.month, d));
        }
    }

    currentDay_ = loadOrCreateDay(calLayout_.year, calLayout_.month, highlightedDay_);
    refreshMonthThumbnails();
}

void CalendarScrapbookApp::shutdown() {
    clearMonthThumbnails();
    for (auto& tex : themePhotos_) {
        gl::deleteTexture(tex);
    }
}

const gl::Texture& CalendarScrapbookApp::textureForDay(const DayLayout& layout) const {
    if (!layout.photoPath.empty() && layout.photoPath != "demo" &&
        layout.photoPath.rfind("theme:", 0) != 0 && photoTex_.id != 0) {
        return photoTex_;
    }
    int theme = parsePhotoTheme(layout.photoPath, layout.day);
    if (themePhotos_[theme].id != 0) {
        return themePhotos_[theme];
    }
    return photoTex_;
}

void CalendarScrapbookApp::clearMonthThumbnails() {
    for (auto& kv : monthThumbs_) {
        gl::deleteTexture(kv.second);
    }
    monthThumbs_.clear();
}

void CalendarScrapbookApp::refreshMonthThumbnails() {
    clearMonthThumbnails();
    if (!fontTex_.id && !photoTex_.id) return;

    for (int d = 1; d <= calLayout_.daysInMonth; ++d) {
        if (hasSavedDayLayout(saveDir_, calLayout_.year, calLayout_.month, d)) {
            DayLayout saved;
            if (loadDayLayout(dayLayoutPath(saveDir_, calLayout_.year, calLayout_.month, d), saved)) {
                Framebuffer thumbFb = renderDayThumbnail(saved, 82, 44);
                monthThumbs_[d] = gl::createTexture(thumbFb, false);
            }
        }
    }
}

void CalendarScrapbookApp::updateDayThumbnail(int day, const DayLayout& layout) {
    if (!fontTex_.id && !photoTex_.id) return;
    auto it = monthThumbs_.find(day);
    if (it != monthThumbs_.end()) {
        gl::deleteTexture(it->second);
        monthThumbs_.erase(it);
    }
    Framebuffer thumbFb = renderDayThumbnail(layout, 82, 44);
    monthThumbs_[day] = gl::createTexture(thumbFb, false);
}

DayLayout CalendarScrapbookApp::loadOrCreateDay(int year, int month, int day) const {
    std::string path = dayLayoutPath(saveDir_, year, month, day);
    DayLayout loaded;
    if (loadDayLayout(path, loaded)) {
        // Upgrade legacy "demo" photo tag to the day's distinct photo theme
        if (loaded.photoPath.empty() || loaded.photoPath == "demo") {
            loaded.photoPath = makePhotoThemeSpec(photoThemeForDay(year, month, day));
        }
        return loaded;
    }
    return makeDefaultDayLayout(year, month, day, defaultPhotoPath_);
}

bool CalendarScrapbookApp::saveCurrentDay() {
    currentDay_.sortByZ();
    std::string path = dayLayoutPath(saveDir_, currentDay_.year, currentDay_.month, currentDay_.day);
    bool ok = saveDayLayout(currentDay_, path);
    if (ok && currentDay_.year == calLayout_.year && currentDay_.month == calLayout_.month) {
        updateDayThumbnail(currentDay_.day, currentDay_);
    }
    return ok;
}

void CalendarScrapbookApp::cycleDayPhotoTheme() {
    int curTheme = parsePhotoTheme(currentDay_.photoPath, currentDay_.day);
    int nextTheme = (curTheme + 1) % kNumPhotoThemes;
    currentDay_.photoPath = makePhotoThemeSpec(nextTheme);
    saveCurrentDay();
}

void CalendarScrapbookApp::selectDayOnCalendar(int day, bool openAfterMove) {
    day = std::clamp(day, 1, calLayout_.daysInMonth);
    if (day == highlightedDay_) {
        if (openAfterMove) openDayPage(day, true);
        return;
    }
    highlightAnim_.active           = true;
    highlightAnim_.fromDay          = highlightedDay_;
    highlightAnim_.toDay            = day;
    highlightAnim_.elapsed          = 0.0;
    highlightAnim_.duration         = openAfterMove ? 0.36 : 0.48;
    highlightAnim_.openPageWhenDone = openAfterMove;
    highlightedDay_                 = day;
}

void CalendarScrapbookApp::openDayPage(int day, bool withPageTurn) {
    day = std::clamp(day, 1, calLayout_.daysInMonth);
    DayLayout prevSnapshot = currentDay_;
    ViewMode  prevView     = view_;

    highlightedDay_ = day;
    currentDay_     = loadOrCreateDay(calLayout_.year, calLayout_.month, day);
    selectedElem_   = -1;
    dragging_       = false;

    saveCurrentDay();

    if (withPageTurn) {
        pageTurnAnim_.active          = true;
        pageTurnAnim_.elapsed         = 0.0;
        pageTurnAnim_.duration        = 0.62;
        pageTurnAnim_.fromView        = prevView;
        pageTurnAnim_.toView          = VIEW_DAY_PAGE;
        pageTurnAnim_.fromDaySnapshot = prevSnapshot;
    }
    view_ = VIEW_DAY_PAGE;
    triggerDropAnimation(-1);
}

void CalendarScrapbookApp::returnToCalendar(bool withPageTurn) {
    if (view_ == VIEW_DAY_PAGE) {
        saveCurrentDay();
        if (withPageTurn) {
            pageTurnAnim_.active          = true;
            pageTurnAnim_.elapsed         = 0.0;
            pageTurnAnim_.duration        = 0.55;
            pageTurnAnim_.fromView        = VIEW_DAY_PAGE;
            pageTurnAnim_.toView          = VIEW_CALENDAR;
            pageTurnAnim_.fromDaySnapshot = currentDay_;
        }
        view_         = VIEW_CALENDAR;
        selectedElem_ = -1;
        dragging_     = false;
    }
}

void CalendarScrapbookApp::navigateMonth(int deltaMonths) {
    int y = calLayout_.year;
    int m = calLayout_.month + deltaMonths;
    while (m < 1)  { m += 12; y -= 1; }
    while (m > 12) { m -= 12; y += 1; }

    calLayout_      = makeMonthLayout(y, m);
    highlightedDay_ = std::clamp(highlightedDay_, 1, calLayout_.daysInMonth);
    highlightAnim_.active = false;
    refreshMonthThumbnails();
}

void CalendarScrapbookApp::triggerDropAnimation(int singleElementIndex) {
    dropAnim_.active           = true;
    dropAnim_.elapsed          = 0.0;
    dropAnim_.duration         = (singleElementIndex >= 0) ? 0.55 : 0.85;
    dropAnim_.singleElementIdx = singleElementIndex;
}

void CalendarScrapbookApp::update(double dt) {
    pulseClock_ += dt;

    if (highlightAnim_.active) {
        highlightAnim_.elapsed += dt;
        if (highlightAnim_.elapsed >= highlightAnim_.duration) {
            highlightAnim_.active = false;
            if (highlightAnim_.openPageWhenDone) {
                highlightAnim_.openPageWhenDone = false;
                openDayPage(highlightedDay_, true);
            }
        }
    }

    if (pageTurnAnim_.active) {
        pageTurnAnim_.elapsed += dt;
        if (pageTurnAnim_.elapsed >= pageTurnAnim_.duration) {
            pageTurnAnim_.active = false;
        }
    }

    if (dropAnim_.active) {
        dropAnim_.elapsed += dt;
        if (dropAnim_.elapsed >= dropAnim_.duration) {
            dropAnim_.active = false;
        }
    }
}

// ---------------------------------------------------------------------------
// Input handling
// ---------------------------------------------------------------------------

void CalendarScrapbookApp::onMouseButton(int button, int action, double mx, double my) {
    mouseX_ = mx;
    mouseY_ = my;
    if (button != 0) return;

    if (action == 0) { // GLFW_RELEASE
        if (dragging_) {
            dragging_ = false;
            saveCurrentDay();
        }
        return;
    }

    if (action != 1) return; // GLFW_PRESS

    if (view_ == VIEW_CALENDAR) {
        // Month navigation buttons 'PREV' and 'NEXT'
        if (insideRect(mx, my, 76, 46, 174, 80)) {
            navigateMonth(-1);
            return;
        }
        if (insideRect(mx, my, 646, 46, 744, 80)) {
            navigateMonth(+1);
            return;
        }

        int clickedDay = dayAt(calLayout_, (int)mx, (int)my);
        if (clickedDay >= 1 && clickedDay <= calLayout_.daysInMonth) {
            if (clickedDay == highlightedDay_) {
                openDayPage(clickedDay, true);
            } else {
                selectDayOnCalendar(clickedDay, true);
            }
        }
        return;
    }

    // ---- VIEW_DAY_PAGE top toolbar buttons ----
    if (insideRect(mx, my, 74, 38, 194, 68)) {
        returnToCalendar(true);
        return;
    }
    if (insideRect(mx, my, 202, 38, 278, 68)) {
        int prevDay = (highlightedDay_ > 1) ? (highlightedDay_ - 1) : calLayout_.daysInMonth;
        openDayPage(prevDay, true);
        return;
    }
    if (insideRect(mx, my, 284, 38, 360, 68)) {
        int nextDay = (highlightedDay_ < calLayout_.daysInMonth) ? (highlightedDay_ + 1) : 1;
        openDayPage(nextDay, true);
        return;
    }
    if (insideRect(mx, my, 368, 38, 462, 68)) {
        cycleDayPhotoTheme();
        return;
    }
    if (insideRect(mx, my, 470, 38, 572, 68)) {
        onKey(70, 1); // 'F' cycle filter
        return;
    }
    if (insideRect(mx, my, 580, 38, 686, 68)) {
        onKey(65, 1); // 'A' add sticker
        return;
    }
    if (insideRect(mx, my, 694, 38, 758, 68)) {
        triggerDropAnimation(-1);
        return;
    }

    // Hit-test stickers front-to-back using inverseMatrix() and pointInPolygon()
    Vec2 clickPt{mx, my};
    int hitIdx = -1;
    for (int i = (int)currentDay_.elements.size() - 1; i >= 0; --i) {
        Sticker st = currentDay_.elements[i].toSticker();
        if (st.hitTest(clickPt)) {
            hitIdx = i;
            break;
        }
    }

    if (hitIdx >= 0) {
        int maxZ = 0;
        for (const auto& e : currentDay_.elements) maxZ = std::max(maxZ, e.zOrder);
        currentDay_.elements[hitIdx].zOrder = maxZ + 1;
        currentDay_.sortByZ();
        selectedElem_ = (int)currentDay_.elements.size() - 1;
        dragging_     = true;
        dragOffset_   = {
            currentDay_.elements[selectedElem_].xform.position.x - mx,
            currentDay_.elements[selectedElem_].xform.position.y - my
        };
        saveCurrentDay();
    } else {
        selectedElem_ = -1;
        dragging_     = false;
    }
}

void CalendarScrapbookApp::onCursorPos(double mx, double my) {
    mouseX_ = mx;
    mouseY_ = my;
    if (view_ == VIEW_CALENDAR) {
        hoveredDay_ = dayAt(calLayout_, (int)mx, (int)my);
        return;
    }
    if (view_ == VIEW_DAY_PAGE && dragging_ &&
        selectedElem_ >= 0 && selectedElem_ < (int)currentDay_.elements.size()) {
        currentDay_.elements[selectedElem_].xform.position = {
            std::clamp(mx + dragOffset_.x, 85.0, 755.0),
            std::clamp(my + dragOffset_.y, 75.0, 525.0)
        };
    }
}

void CalendarScrapbookApp::onKey(int key, int action) {
    if (action != 1 && action != 2) return;

    if (view_ == VIEW_CALENDAR) {
        if (key == 263) { // Left
            selectDayOnCalendar(std::max(1, highlightedDay_ - 1), false);
        } else if (key == 262) { // Right
            selectDayOnCalendar(std::min(calLayout_.daysInMonth, highlightedDay_ + 1), false);
        } else if (key == 265) { // Up
            selectDayOnCalendar(std::max(1, highlightedDay_ - 7), false);
        } else if (key == 264) { // Down
            selectDayOnCalendar(std::min(calLayout_.daysInMonth, highlightedDay_ + 7), false);
        } else if (key == 257 || key == 335) { // Enter
            openDayPage(highlightedDay_, true);
        } else if (key == 44 || key == 91) { // ',' or '['
            navigateMonth(-1);
        } else if (key == 46 || key == 93) { // '.' or ']'
            navigateMonth(+1);
        }
        return;
    }

    // ---- VIEW_DAY_PAGE keys ----
    if (key == 66 || key == 258) { // 'B' or Tab -> Back to Calendar
        returnToCalendar(true);
        return;
    }
    if (key == 78) { // 'N' -> Next day page-turn
        int nextDay = (highlightedDay_ < calLayout_.daysInMonth) ? (highlightedDay_ + 1) : 1;
        openDayPage(nextDay, true);
        return;
    }
    if (key == 80) { // 'P' -> Previous day page-turn
        int prevDay = (highlightedDay_ > 1) ? (highlightedDay_ - 1) : calLayout_.daysInMonth;
        openDayPage(prevDay, true);
        return;
    }
    if (key == 67) { // 'C' -> Cycle daily photo scene
        cycleDayPhotoTheme();
        return;
    }
    if (key == 68) { // 'D' -> Replay live drop-and-settle animation
        triggerDropAnimation(-1);
        return;
    }
    if (key == 83) { // 'S' -> Explicit save
        saveCurrentDay();
        std::cout << "[Option 5] Saved day layout to "
                  << dayLayoutPath(saveDir_, currentDay_.year, currentDay_.month, currentDay_.day) << "\n";
        return;
    }
    if (key == 76) { // 'L' -> Reload from file
        currentDay_ = loadOrCreateDay(currentDay_.year, currentDay_.month, currentDay_.day);
        triggerDropAnimation(-1);
        return;
    }
    if (key == 70) { // 'F' -> Cycle photo filter on the day page
        currentDay_.photoFilter = (currentDay_.photoFilter + 1) % 7;
        currentDay_.filterStrength = (currentDay_.photoFilter == 0) ? 0.0f : 0.75f;
        saveCurrentDay();
        return;
    }
    if (key == 65) { // 'A' -> Add a new sticker with live drop-and-settle
        static const ShapeKind kPalette[] = {
            ShapeKind::STAR, ShapeKind::BEZIER_HEART, ShapeKind::CURVY_STAR,
            ShapeKind::RIBBON, ShapeKind::CLOUD, ShapeKind::HEXAGON
        };
        static const Color kFills[] = {
            kMustard, kTerracotta, kHoney, kGreen, kWhite, kSage
        };
        int idx = (int)currentDay_.elements.size() % 6;
        int maxZ = 0;
        for (const auto& e : currentDay_.elements) maxZ = std::max(maxZ, e.zOrder);

        SavedElement ne;
        ne.kind           = kPalette[idx];
        ne.xform.pivot    = {0, 0};
        ne.xform.position = {490.0 + (idx % 3) * 75.0, 165.0 + (idx % 4) * 65.0};
        ne.xform.rotation = (idx % 2 == 0) ? 12.0 : -14.0;
        ne.xform.scale    = {1.0, 1.0};
        ne.fill           = kFills[idx];
        ne.edge           = kBrown;
        ne.zOrder         = maxZ + 1;

        currentDay_.elements.push_back(ne);
        selectedElem_ = (int)currentDay_.elements.size() - 1;
        saveCurrentDay();
        triggerDropAnimation(selectedElem_);
        return;
    }

    if (selectedElem_ >= 0 && selectedElem_ < (int)currentDay_.elements.size()) {
        SavedElement& sel = currentDay_.elements[selectedElem_];
        bool changed = false;
        if (key == 81) { // 'Q' rotate counter-clockwise
            sel.xform.rotation -= 8.0;
            changed = true;
        } else if (key == 69) { // 'E' rotate clockwise
            sel.xform.rotation += 8.0;
            changed = true;
        } else if (key == 61 || key == 334) { // '=' / '+' scale up
            sel.xform.scale.x = std::min(2.5, sel.xform.scale.x * 1.1);
            sel.xform.scale.y = std::min(2.5, sel.xform.scale.y * 1.1);
            changed = true;
        } else if (key == 45 || key == 333) { // '-' scale down
            sel.xform.scale.x = std::max(0.35, sel.xform.scale.x * 0.9);
            sel.xform.scale.y = std::max(0.35, sel.xform.scale.y * 0.9);
            changed = true;
        } else if (key == 261 || key == 259) { // Delete or Backspace
            currentDay_.elements.erase(currentDay_.elements.begin() + selectedElem_);
            selectedElem_ = -1;
            changed = true;
        }
        if (changed) saveCurrentDay();
    }
}

// ---------------------------------------------------------------------------
// Cohesive Scrapbook Notebook Shell & OpenGL Rendering
// ---------------------------------------------------------------------------

void CalendarScrapbookApp::renderNotebookShell() {
    // 1. Warm kraft desk surface + dark leather left binder spine
    drawRectGL(0, 0, 800, 600, kBrown);
    drawRectGL(0, 0, 46, 600, kLeather);
    drawRectGL(44, 0, 48, 600, Color{72, 46, 28, 255});

    // 2. Multi-layered page card drop shadow + cream journal card
    drawRectGL(62, 32, 782, 582, Color{84, 54, 34, 140});
    drawRectGL(58, 28, 778, 578, kShadow);
    drawRectGL(52, 22, 772, 572, kCream, kBrown);

    // Inner stitched journal frame
    gl::drawPolyline(rectPoly(60, 30, 764, 564), Mat3::identity(), Color{214, 174, 118, 255});

    // 3. Binder holes + metallic spiral rings connecting the spine to the page
    Polygon hole = makeRegularPolygon({0, 0}, 9, 20);
    for (int y = 62; y <= 542; y += 68) {
        gl::drawPolygon(hole, Mat3::translate(26, y), kDark);
        gl::drawPolygon(hole, Mat3::translate(68, y), kDark);
        // Spiral ring bar bridging spine hole to page hole
        drawRectGL(22, y - 4, 72, y + 4, Color{224, 214, 196, 255}, kDark);
        drawRectGL(24, y - 2, 70, y,      kWhite);
    }
}

void CalendarScrapbookApp::render() {
    if (pageTurnAnim_.active) {
        if (pageTurnAnim_.toView == VIEW_DAY_PAGE) {
            renderDayPageView(currentDay_, true);
        } else {
            renderCalendarView();
        }
        renderPageTurnOverlay();
        return;
    }

    if (view_ == VIEW_CALENDAR) {
        renderCalendarView();
    } else {
        renderDayPageView(currentDay_, true);
    }
}

void CalendarScrapbookApp::renderCalendarView() {
    renderNotebookShell();

    // 1. Cohesive Header Banner + Month Navigation Buttons
    bool prevHover = insideRect(mouseX_, mouseY_, 76, 46, 174, 80);
    bool nextHover = insideRect(mouseX_, mouseY_, 646, 46, 744, 80);
    drawButtonGL(fontTex_, 76, 46, 174, 80, "PREV", kPaper, kDark, prevHover);
    drawButtonGL(fontTex_, 646, 46, 744, 80, "NEXT", kPaper, kDark, nextHover);

    // Center ribbon banner for month title
    Polygon banner = makeRibbon({410, 64}, 430, 44, 14);
    drawConcavePolygonGL(banner, Mat3::translate(3, 4), Color{77, 74, 63, 75});
    drawConcavePolygonGL(banner, Mat3::identity(), kHoney);
    gl::drawPolyline(banner, Mat3::identity(), kBrown);

    std::string title = monthTitle(calLayout_.year, calLayout_.month);
    drawCrispTextCentered(fontTex_, title, 410, 58, 3, kDark, true, kWhite);
    drawCrispTextCentered(fontTex_, "DAILY PHOTO JOURNAL", 410, 76, 1, kBrown);

    // 2. Weekday Column Header Bar (3-letter names at crisp integer scale = 2)
    static const char* kWeekdays[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    for (int c = 0; c < 7; ++c) {
        double x0 = calLayout_.x0 + c * calLayout_.cw;
        double y0 = calLayout_.y0 - calLayout_.hdr;
        Color hdrBg = (c == 0 || c == 6) ? kDeepGreen : kGreen;
        drawRectGL(x0, y0, x0 + calLayout_.cw, calLayout_.y0, hdrBg, kBrown);
        drawCrispTextCentered(fontTex_, kWeekdays[c],
                              x0 + calLayout_.cw * 0.5, y0 + calLayout_.hdr * 0.5,
                              2, kWhite, true, kDark);
    }

    // 3. Calendar Grid Cells
    const int numRows = calLayout_.rows();
    for (int r = 0; r < numRows; ++r) {
        for (int c = 0; c < 7; ++c) {
            double x0 = calLayout_.x0 + c * calLayout_.cw;
            double y0 = calLayout_.y0 + r * calLayout_.ch;
            Color cellCol = (c == 0 || c == 6) ? kWeekend : kPaper;
            drawRectGL(x0, y0, x0 + calLayout_.cw, y0 + calLayout_.ch, cellCol, kBrown);
        }
    }

    // 4. Saved-Page Photo Thumbnails, Hover Highlight, and Crisp Day Numbers
    int starCoveredDay = highlightAnim_.active ? 0 : highlightedDay_;
    for (int d = 1; d <= calLayout_.daysInMonth; ++d) {
        int x0, y0, x1, y1;
        cellRect(calLayout_, d, x0, y0, x1, y1);

        // Hover highlight when mouse is over this cell
        if (d == hoveredDay_ && !highlightAnim_.active) {
            drawRectGL(x0 + 2, y0 + 2, x1 - 2, y1 - 2, Color{249, 205, 125, 110}, kTerracotta);
        }

        // Draw saved layout thumbnail (mini Polaroid with that day's photo + stickers)
        auto thumbIt = monthThumbs_.find(d);
        if (thumbIt != monthThumbs_.end() && thumbIt->second.id != 0) {
            double tx = x0 + 6.0;
            double ty = y0 + 18.0;
            double tw = calLayout_.cw - 12.0;
            double th = std::max(20.0, (double)calLayout_.ch - 22.0);
            drawRectGL(tx + 2, ty + 2, tx + tw + 2, ty + th + 2, Color{64, 58, 48, 65});
            Mat3 thumbM = Mat3::translate(tx, ty)
                        * Mat3::scale(tw / (double)thumbIt->second.width,
                                      th / (double)thumbIt->second.height);
            gl::drawTexture(thumbIt->second, thumbM, 1.0f);

            // Corner sticker badge showing a saved scrapbook entry
            Polygon badge = (d % 2 == 0) ? makeStar({0, 0}, 8, 4) : makeHeart({0, 0}, 13);
            Color badgeCol = (d % 2 == 0) ? kMustard : kTerracotta;
            drawConcavePolygonGL(badge, Mat3::translate(x1 - 11.0, y0 + 10.0), badgeCol);
        }

        // Crisp integer scale=2 day number in top-left of cell
        if (d != starCoveredDay) {
            std::string dStr = std::to_string(d);
            int nw = crispTextWidth(dStr, 2);
            drawRectGL(x0 + 3, y0 + 3, x0 + 7 + nw, y0 + 16, Color{255, 252, 245, 210});
            drawCrispText(fontTex_, dStr, x0 + 5, y0 + 5, 2, kDark);
        }
    }

    // 5. Highlighted Day Star Marker + Smooth Highlight Move Animation
    Polygon outerStar = makeStar({0, 0}, 28, 13);
    Polygon innerStar = makeStar({0, 0}, 19, 9);

    if (highlightAnim_.active) {
        double rawT = easing::clamp01(highlightAnim_.elapsed / highlightAnim_.duration);
        double u    = easing::easeInOutCubic(rawT);

        int tx0, ty0, tx1, ty1;
        cellRect(calLayout_, highlightAnim_.toDay, tx0, ty0, tx1, ty1);
        drawRectGL(tx0 + 2, ty0 + 2, tx1 - 2, ty1 - 2,
                   Color{249, 205, 125, (uint8_t)(160 * u)});

        for (int k = 5; k >= 1; --k) {
            double trailT = rawT - k * 0.04;
            if (trailT <= 0.0) continue;
            Vec2 tp = highlightCentre(calLayout_, highlightAnim_.fromDay, highlightAnim_.toDay, trailT);
            double sc = 0.55 * (1.0 - k * 0.08);
            uint8_t alpha = (uint8_t)(95.0 * (1.0 - k / 6.0) * (1.0 - rawT * 0.5));
            Mat3 tm = Mat3::translate(tp.x, tp.y)
                    * Mat3::rotate(2.0 * M_PI * smoothstep01(trailT))
                    * Mat3::scale(sc, sc);
            drawConcavePolygonGL(outerStar, tm, Color{228, 165, 69, alpha});
        }

        Vec2 pos = highlightCentre(calLayout_, highlightAnim_.fromDay, highlightAnim_.toDay, rawT);
        double lift = std::sin(M_PI * u);
        double scale = 1.0 + 0.30 * lift;
        double angle = 2.0 * M_PI * u;

        Mat3 shadowM = Mat3::translate(pos.x + 4.0 + 14.0 * lift, pos.y + 5.0 + 18.0 * lift)
                     * Mat3::rotate(angle) * Mat3::scale(scale, scale);
        drawConcavePolygonGL(outerStar, shadowM, Color{64, 58, 48, 90});

        Mat3 starM = Mat3::translate(pos.x, pos.y) * Mat3::rotate(angle) * Mat3::scale(scale, scale);
        drawConcavePolygonGL(outerStar, starM, kMustard);
        gl::drawPolyline(outerStar, starM, Color{150, 100, 30, 255});
        drawConcavePolygonGL(innerStar, starM, kHoney);

        double burstT = segmentT(rawT, 0.75, 1.0);
        if (burstT > 0.0 && burstT < 1.0) {
            Vec2 destC = cellCentre(calLayout_, highlightAnim_.toDay);
            Polygon miniSq = makeSquare({0, 0}, 7);
            const Color cols[4] = {kMustard, kTerracotta, kGreen, kHoney};
            for (int i = 0; i < 12; ++i) {
                double ang  = 2.0 * M_PI * hash01(17 + i * 3);
                double dist = (30.0 + 55.0 * hash01(18 + i * 3)) * easing::easeOutCubic(burstT);
                Vec2 cp{destC.x + std::cos(ang) * dist,
                        destC.y + std::sin(ang) * dist + 25.0 * burstT * burstT};
                uint8_t ca = (uint8_t)(220.0 * (1.0 - burstT));
                Color cc = cols[i % 4];
                cc.a = ca;
                gl::drawPolygon(miniSq, Mat3::translate(cp.x, cp.y) * Mat3::rotate(burstT * 4.0), cc);
            }
        }
    } else {
        int hx0, hy0, hx1, hy1;
        cellRect(calLayout_, highlightedDay_, hx0, hy0, hx1, hy1);
        gl::drawPolyline(rectPoly(hx0 + 1, hy0 + 1, hx1 - 1, hy1 - 1), Mat3::identity(), kTerracotta);

        // Place the star badge near top-left/center so any saved photo thumbnail underneath stays visible
        Vec2 hc{hx0 + 26.0, hy0 + 24.0};
        if (monthThumbs_.find(highlightedDay_) == monthThumbs_.end()) {
            hc = cellCentre(calLayout_, highlightedDay_);
        }
        double pulse = 1.0 + 0.05 * std::sin(pulseClock_ * 3.5);
        Mat3 shadowM = Mat3::translate(hc.x + 3.0, hc.y + 4.0) * Mat3::scale(pulse, pulse);
        Mat3 starM   = Mat3::translate(hc.x, hc.y) * Mat3::scale(pulse, pulse);
        drawConcavePolygonGL(outerStar, shadowM, Color{64, 58, 48, 85});
        drawConcavePolygonGL(outerStar, starM, kMustard);
        gl::drawPolyline(outerStar, starM, Color{150, 100, 30, 255});
        drawConcavePolygonGL(innerStar, starM, kHoney);

        drawCrispTextCentered(fontTex_, std::to_string(highlightedDay_), hc.x, hc.y, 2, kDark, true, kWhite);
    }

    // 6. Cohesive Bottom Status Ribbon (crisp integer scale = 2)
    drawRectGL(70, 536, 756, 560, kDeepGreen, kBrown);
    int theme = photoThemeForDay(calLayout_.year, calLayout_.month, highlightedDay_);
    std::string footer = "DAY " + std::to_string(highlightedDay_) + " - " + photoThemeCaption(theme)
                       + " - CLICK OR ENTER TO OPEN PAGE";
    drawCrispTextCentered(fontTex_, footer, 413, 548, 2, kWhite, true, kDark);
}

void CalendarScrapbookApp::renderDayPageView(const DayLayout& layout, bool applyDropAnim) {
    renderNotebookShell();

    // 1. Top Cohesive Toolbar with Hover States (crisp integer scale = 2)
    drawButtonGL(fontTex_,  74, 38, 194, 68, "CALENDAR", kGreen,      kWhite, insideRect(mouseX_, mouseY_,  74, 38, 194, 68));
    drawButtonGL(fontTex_, 202, 38, 278, 68, "PREV",     kPaper,      kDark,  insideRect(mouseX_, mouseY_, 202, 38, 278, 68));
    drawButtonGL(fontTex_, 284, 38, 360, 68, "NEXT",     kPaper,      kDark,  insideRect(mouseX_, mouseY_, 284, 38, 360, 68));
    drawButtonGL(fontTex_, 368, 38, 462, 68, "PHOTO",    kHoney,      kDark,  insideRect(mouseX_, mouseY_, 368, 38, 462, 68));
    drawButtonGL(fontTex_, 470, 38, 572, 68, "FILTER",   kSage,       kDark,  insideRect(mouseX_, mouseY_, 470, 38, 572, 68));
    drawButtonGL(fontTex_, 580, 38, 686, 68, "STICKER",  kMustard,    kDark,  insideRect(mouseX_, mouseY_, 580, 38, 686, 68));
    drawButtonGL(fontTex_, 694, 38, 758, 68, "DROP",     kTerracotta, kWhite, insideRect(mouseX_, mouseY_, 694, 38, 758, 68));

    // 2. Left Side: Polaroid Photo Card with This Day's Distinct Photograph
    drawRectGL(110, 92, 466, 456, Color{64, 58, 48, 75});
    drawRectGL(102, 84, 458, 448, kWhite, kBrown);

    const gl::Texture& dayTex = textureForDay(layout);
    if (dayTex.id != 0) {
        FilterParams fp;
        fp.kind     = (FilterKind)std::clamp(layout.photoFilter, 0, (int)FILTER_COUNT - 1);
        fp.strength = layout.filterStrength;
        fp.amount   = (fp.kind == FILTER_CONTRAST) ? 1.45f : 0.18f;
        fp.tint     = {235, 165, 120, 255};
        Mat3 photoM = Mat3::translate(118.0, 98.0)
                    * Mat3::scale(324.0 / (double)dayTex.width,
                                  264.0 / (double)dayTex.height);
        gl::drawTexture(dayTex, photoM, 1.0f, fp);
    }
    gl::drawPolyline(rectPoly(118, 98, 442, 362), Mat3::identity(), kDark);

    // Polaroid Bottom Caption Area (Scene Title at scale=3 + Date/Filter at scale=2)
    int theme = parsePhotoTheme(layout.photoPath, layout.day);
    std::string sceneTitle = photoThemeCaption(theme);
    drawCrispTextCentered(fontTex_, sceneTitle, 280, 388, 3, kDark, true, kCream);

    std::string subCaption = layout.dateText + "   FILTER " + shortFilterLabel(layout.photoFilter);
    drawCrispTextCentered(fontTex_, subCaption, 280, 422, 2, kBrown);

    // 3. Right Side: Lined Journal Note Card Backdrop for Stickers
    drawRectGL(482, 92, 754, 424, Color{64, 58, 48, 55});
    drawRectGL(476, 86, 748, 418, kPaper, kBrown);
    // Red vertical margin line & horizontal ruled notebook lines
    drawRectGL(506, 86, 508, 418, Color{216, 130, 118, 160});
    for (int ry = 132; ry <= 396; ry += 24) {
        drawRectGL(478, ry, 746, ry + 1, Color{195, 205, 215, 160});
    }
    std::string noteHdr = "DAY " + std::to_string(layout.day) + " JOURNAL";
    drawCrispTextCentered(fontTex_, noteHdr, 612, 108, 2, kDeepGreen, true, kWhite);

    // 4. Z-ordered Stickers, Washi Tape & Notched Date Stamp with Live Drop-and-Settle
    double dropT = (applyDropAnim && dropAnim_.active)
                       ? easing::clamp01(dropAnim_.elapsed / dropAnim_.duration)
                       : 1.0;
    const int total = (int)layout.elements.size();

    for (int i = 0; i < total; ++i) {
        const SavedElement& elem = layout.elements[i];
        Transform xf = elem.xform;
        if (applyDropAnim && dropAnim_.active) {
            if (dropAnim_.singleElementIdx < 0) {
                xf = evaluateDropAndSettle(elem.xform, dropT, i, total);
            } else if (dropAnim_.singleElementIdx == i) {
                xf = evaluateDropAndSettle(elem.xform, dropT, 0, 1);
            }
        }

        Polygon base = buildShapePolygon(elem.kind);
        Mat3 M = xf.toMat3();

        if (elem.fill.a >= 240) {
            double liftOffset = (applyDropAnim && dropAnim_.active) ? (1.0 - dropT) * 14.0 : 0.0;
            Mat3 shadowM = Mat3::translate(5.0 + liftOffset, 6.0 + liftOffset) * M;
            drawConcavePolygonGL(base, shadowM, Color{64, 58, 48, 85});
        }

        drawConcavePolygonGL(base, M, elem.fill);
        if (elem.edge.a > 0) {
            gl::drawPolyline(base, M, elem.edge);
        }

        // Crisp date stamp text inside the DATE_STAMP element
        if (elem.kind == ShapeKind::DATE_STAMP) {
            std::string dtxt = layout.dateText.empty()
                                   ? formatDateStamp(layout.year, layout.month, layout.day)
                                   : layout.dateText;
            float sc = 3.0f;
            float tw = (float)crispTextWidth(dtxt, 3);
            gl::drawStampText(fontTex_, dtxt, -tw * 0.5f + 1.0f, -7.5f + 1.0f, sc, kCream, M);
            gl::drawStampText(fontTex_, dtxt, -tw * 0.5f,        -7.5f,        sc, kTerracotta, M);
        }

        // Selection frame around active sticker
        if (i == selectedElem_ && !pageTurnAnim_.active) {
            Polygon wp = transformPolygon(base, M);
            Vec2 mn, mx;
            boundingBox(wp, mn, mx);
            Polygon selBox = rectPoly(mn.x - 5, mn.y - 5, mx.x + 5, mx.y + 5);
            gl::drawPolyline(selBox, Mat3::identity(), kTerracotta);
            drawRectGL(mx.x + 1, mx.y + 1, mx.x + 9, mx.y + 9, kTerracotta, kWhite);
        }
    }

    // 5. Cohesive Bottom Status Ribbon (crisp integer scale = 2)
    drawRectGL(74, 536, 756, 560, kDeepGreen, kBrown);
    drawCrispTextCentered(fontTex_,
                          "DRAG STICKER   Q E ROTATE   C PHOTO   F FILTER   B CALENDAR",
                          415, 548, 2, kWhite, true, kDark);
}

void CalendarScrapbookApp::renderPageTurnOverlay() {
    double rawT = easing::clamp01(pageTurnAnim_.elapsed / pageTurnAnim_.duration);
    Polygon clipRegion = computePageTurnClipRegion(rawT, 800.0, 600.0);
    Polygon flapPoly   = computePageTurnFlap(rawT, 800.0, 600.0);

    double eased = easing::easeInOutCubic(rawT);
    std::vector<motion::Point2> foldPts = motion::pageCurlEdge(eased, 800.0, 600.0, 24);
    Polygon foldLine;
    foldLine.reserve(foldPts.size());
    for (const auto& p : foldPts) foldLine.push_back({p.x, p.y});

    Color outgoingSurface = (pageTurnAnim_.fromView == VIEW_CALENDAR) ? kCream : kPaper;
    drawConcavePolygonGL(clipRegion, Mat3::identity(), outgoingSurface);

    drawConcavePolygonGL(flapPoly, Mat3::translate(-18.0, 0.0), Color{30, 22, 15, 85});

    drawConcavePolygonGL(flapPoly, Mat3::identity(), Color{242, 220, 182, 245});
    gl::drawPolyline(flapPoly, Mat3::identity(), kBrown);
    gl::drawPolyline(foldLine, Mat3::identity(), kDark);
}
