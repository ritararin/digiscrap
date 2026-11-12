#pragma once
// ---------------------------------------------------------------------------
// calendar.h — Calendar grid layout, hit-testing, month navigation, and
// software-rendered CP2 calendar animations.
// ---------------------------------------------------------------------------

#include <string>
#include "core/framebuffer.h"
#include "core/mat3.h"

constexpr int kFrameW = 800;
constexpr int kFrameH = 600;

inline Color pixelAt(const Framebuffer& f, int x, int y) {
    if (x < 0 || y < 0 || x >= f.width || y >= f.height) return {0, 0, 0, 0};
    return f.pixels[y * f.width + x];
}

struct CalendarLayout {
    int x0          = 70;
    int y0          = 160;
    int cw          = 94;
    int ch          = 68;
    int hdr         = 34;
    int startCol    = 4;     // 1 Oct 2026 is a Thursday (Sunday = 0)
    int daysInMonth = 31;
    int year        = 2026;
    int month       = 10;    // 1..12

    int rows() const { return (startCol + daysInMonth + 6) / 7; }
};

// ---- Month navigation helpers (Final Project) -----------------------------
// Returns number of days in a month (1..12), accounting for leap years.
inline int daysInMonthFor(int year, int month) {
    static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 30;
    if (month == 2) {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return leap ? 29 : 28;
    }
    return kDays[month - 1];
}

// Sakamoto's algorithm: day of week for the 1st of (year, month), 0 = Sunday .. 6 = Saturday.
inline int firstWeekdayOfMonth(int year, int month) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = year;
    if (month < 3) y -= 1;
    return (y + y / 4 - y / 100 + y / 400 + t[month - 1] + 1) % 7;
}

inline CalendarLayout makeMonthLayout(int year, int month) {
    CalendarLayout L;
    L.year        = year;
    L.month       = month;
    L.startCol    = firstWeekdayOfMonth(year, month);
    L.daysInMonth = daysInMonthFor(year, month);
    // Keep 6-row months neatly inside the card [160..540]
    if (L.rows() > 5) L.ch = 58;
    return L;
}

inline std::string monthName(int month) {
    static const char* kNames[12] = {
        "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
        "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
    };
    if (month < 1 || month > 12) return "MONTH";
    return kNames[month - 1];
}

inline std::string monthTitle(int year, int month) {
    return monthName(month) + " " + std::to_string(year);
}

// ---- Grid geometry & hit-testing ------------------------------------------
double smoothstep01(double t);
double pulseRadius(double base, double t, int pulses);

void cellRect(const CalendarLayout& L, int day, int& x0, int& y0, int& x1, int& y1);
Vec2 cellCentre(const CalendarLayout& L, int day);
int  dayAt(const CalendarLayout& L, int px, int py);

// ---- Software calendar rendering & CP2 animations -------------------------
void drawCalendarBackdrop(Framebuffer& fb, const CalendarLayout& L);
void drawDayNumbers(Framebuffer& fb, const CalendarLayout& L, int visible, int skipDay = 0);
void drawGridProgress(Framebuffer& fb, const CalendarLayout& L, double t);
void drawCalendarFull(Framebuffer& fb, const CalendarLayout& L);

Vec2 highlightCentre(const CalendarLayout& L, int fromDay, int toDay, double t);
void drawHighlightMove(Framebuffer& fb, const CalendarLayout& L, int fromDay, int toDay, double t);

void drawPulsingCircle(Framebuffer& fb, Vec2 c, double baseR, double t, int pulses,
                       Color fill, Color edge);
void drawPulsingEllipse(Framebuffer& fb, Vec2 c, double baseRx, double baseRy, double t,
                        int pulses, Color fill, Color edge);

void drawZoomTransition(Framebuffer& dst, const Framebuffer& calendarFrame,
                        const Framebuffer& dayFrame, const CalendarLayout& L, int day, double t);

void renderCalendarGrid(Framebuffer& fb, double t);
void renderCalendarSelect(Framebuffer& fb, double t);
void renderCalendarPulse(Framebuffer& fb, double t);
void setZoomDayPage(const Framebuffer* dayPage);
void renderCalendarZoom(Framebuffer& fb, double t);
