#pragma once
// ---------------------------------------------------------------------------
// calendar_gl.h — Interactive OpenGL Calendar Month View, Multi-Day Page
// Editor, Per-Day Illustrated Photographs, Live CP2 Animations (drop-and-settle,
// page-turn, highlight move), Thumbnails of Saved Pages, and Autosave.
// ---------------------------------------------------------------------------

#include <array>
#include <map>
#include <string>
#include <vector>
#include "calendar/calendar.h"
#include "calendar/day_storage.h"
#include "opengl/texture_renderer.h"

// ---- Pure animation helpers (testable without an OpenGL context) ----------

// Evaluate the drop-and-settle Transform for element `index` (out of `totalCount`)
// at normalized animation time t in [0, 1].
// Uses easing::easeOutBounce for vertical drop and easing::easeOutBack for scale settle.
// At t = 1.0, returns `target` exactly.
Transform evaluateDropAndSettle(const Transform& target, double t,
                                int index = 0, int totalCount = 1);

// Compute the visible region polygon of the outgoing page during a page-turn
// at normalized time t in [0, 1], using easing::easeInOutCubic + motion::pageCurlRegion.
Polygon computePageTurnClipRegion(double t, double width = 800.0, double height = 600.0);

// Compute the curled back-flap polygon along the fold edge at time t in [0, 1],
// using motion::pageCurlEdge.
Polygon computePageTurnFlap(double t, double width = 800.0, double height = 600.0);

// Ear-clipping triangulation so concave shapes (star, heart, cloud, date stamp,
// page-curl region) render cleanly as triangles in OpenGL 3.3 Core.
std::vector<Polygon> triangulateConcave(const Polygon& poly);

// ---- Interactive OpenGL Multi-Day Scrapbook Controller --------------------

class CalendarScrapbookApp {
public:
    enum ViewMode {
        VIEW_CALENDAR = 0,
        VIEW_DAY_PAGE = 1
    };

    CalendarScrapbookApp();
    ~CalendarScrapbookApp();

    // Initialise GPU resources and load/seed saved day layouts from `saveDir`.
    void init(const gl::Texture& photoTex, const gl::Texture& fontTex,
              const std::string& saveDir = "saves",
              const std::string& defaultPhotoPath = "demo");

    void shutdown();

    // Per-frame update and OpenGL render.
    void update(double dt);
    void render();

    // Input callbacks (coordinates in 800x600 logical space).
    void onMouseButton(int button, int action, double mx, double my);
    void onCursorPos(double mx, double my);
    void onKey(int key, int action);

    // Programmatic actions (useful for demos and tests).
    void selectDayOnCalendar(int day, bool openAfterMove = false);
    void openDayPage(int day, bool withPageTurn = true);
    void returnToCalendar(bool withPageTurn = true);
    void navigateMonth(int deltaMonths);
    void cycleDayPhotoTheme();
    void triggerDropAnimation(int singleElementIndex = -1);
    bool saveCurrentDay();

    ViewMode              viewMode() const { return view_; }
    int                   highlightedDay() const { return highlightedDay_; }
    const CalendarLayout& calendarLayout() const { return calLayout_; }
    const DayLayout&      currentDayLayout() const { return currentDay_; }

private:
    struct HighlightMoveState {
        bool   active           = false;
        int    fromDay          = 5;
        int    toDay            = 5;
        double elapsed          = 0.0;
        double duration         = 0.48;
        bool   openPageWhenDone = false;
    };

    struct DropSettleState {
        bool   active            = false;
        double elapsed           = 0.0;
        double duration          = 0.85;
        int    singleElementIdx  = -1; // -1 = animate all elements on page
    };

    struct PageTurnState {
        bool      active       = false;
        double    elapsed      = 0.0;
        double    duration     = 0.65;
        ViewMode  fromView     = VIEW_CALENDAR;
        ViewMode  toView       = VIEW_DAY_PAGE;
        DayLayout fromDaySnapshot;
    };

    gl::Texture photoTex_{};
    gl::Texture fontTex_{};
    std::array<gl::Texture, kNumPhotoThemes> themePhotos_{};
    std::string saveDir_          = "saves";
    std::string defaultPhotoPath_ = "demo";

    ViewMode       view_           = VIEW_CALENDAR;
    CalendarLayout calLayout_{};
    int            highlightedDay_ = 5;
    int            hoveredDay_     = 0;
    double         mouseX_         = 0.0;
    double         mouseY_         = 0.0;
    double         pulseClock_     = 0.0;

    DayLayout      currentDay_{};
    int            selectedElem_   = -1;
    bool           dragging_       = false;
    Vec2           dragOffset_{0, 0};

    HighlightMoveState highlightAnim_{};
    DropSettleState    dropAnim_{};
    PageTurnState      pageTurnAnim_{};

    // Cached GPU textures for thumbnails of saved days in `calLayout_.month` (key = day 1..31).
    std::map<int, gl::Texture> monthThumbs_;

    const gl::Texture& textureForDay(const DayLayout& layout) const;
    void refreshMonthThumbnails();
    void updateDayThumbnail(int day, const DayLayout& layout);
    void clearMonthThumbnails();
    DayLayout loadOrCreateDay(int year, int month, int day) const;

    void renderNotebookShell();
    void renderCalendarView();
    void renderDayPageView(const DayLayout& layout, bool applyDropAnim);
    void renderPageTurnOverlay();
};
