#pragma once
// ---------------------------------------------------------------------------
// day_storage.h — Multi-day layout model, plain-text serializer/loader,
// per-day procedural/file photo generator, and software thumbnail renderer.
// ---------------------------------------------------------------------------

#include <string>
#include <vector>
#include "core/framebuffer.h"
#include "core/mat3.h"
#include "animation/transform.h"
#include "stickers/sticker.h"

enum class ShapeKind {
    STAR = 0,
    HEART,
    BEZIER_HEART,
    SQUARE,
    HEXAGON,
    CIRCLE,
    ELLIPSE,
    CLOUD,
    CURVY_STAR,
    RIBBON,
    TAPE,
    DATE_STAMP
};

constexpr int kNumPhotoThemes = 8;

// Convert between ShapeKind and its text token for layout files.
std::string shapeKindToString(ShapeKind kind);
ShapeKind   shapeKindFromString(const std::string& token, bool* ok = nullptr);

// Build the rest-pose polygon centered at (0, 0) using our CP1/CP2 generators.
Polygon buildShapePolygon(ShapeKind kind);

// One saved element on a day's scrapbook page.
struct SavedElement {
    ShapeKind kind   = ShapeKind::STAR;
    Transform xform;
    Color     fill   {228, 165, 69, 255};
    Color     edge   {150, 100, 30, 255};
    int       zOrder = 0;

    Sticker toSticker() const {
        Sticker s;
        s.baseShape = buildShapePolygon(kind);
        s.xform     = xform;
        s.fill      = fill;
        s.edge      = edge;
        s.zOrder    = zOrder;
        return s;
    }
};

// Complete layout for a single calendar day.
struct DayLayout {
    int         year           = 2026;
    int         month          = 10;
    int         day            = 5;
    std::string photoPath      = "theme:4";
    int         photoFilter    = 0;      // 0 = FILTER_NONE (shows vivid daily photo by default; F cycles sepia/etc.)
    float       filterStrength = 1.0f;
    std::string dateText       = "05-10-2026";
    std::vector<SavedElement> elements;

    void sortByZ();
};

// Format "DD-MM-YYYY" for a date stamp.
std::string formatDateStamp(int year, int month, int day);

// Daily photo theme helpers (0 .. kNumPhotoThemes-1):
//   0: SUNSET HIKE    1: STARRY CAMP    2: OCEAN BREEZE   3: AUTUMN WALK
//   4: CAFE MORNING   5: FLOWER FIELD   6: CITY LIGHTS    7: RAINBOW DAY
int         photoThemeForDay(int year, int month, int day);
int         parsePhotoTheme(const std::string& photoPath, int fallbackDay = 1);
std::string makePhotoThemeSpec(int themeIndex);
std::string photoThemeCaption(int themeIndex);
Framebuffer makeDayPhoto(int themeIndex, int w = 360, int h = 270);
Framebuffer resolveDayPhoto(const DayLayout& layout, int w = 360, int h = 270);

// Build a rich default scrapbook layout for (year, month, day).
DayLayout makeDefaultDayLayout(int year, int month, int day,
                               const std::string& customPhotoPath = "");

// Plain-text serialization (in-memory string and file I/O).
std::string serializeDayLayout(const DayLayout& layout);
bool        deserializeDayLayout(const std::string& text, DayLayout& outLayout);

bool        saveDayLayout(const DayLayout& layout, const std::string& filePath);
bool        loadDayLayout(const std::string& filePath, DayLayout& outLayout);

// Standard path helper: "<saveDir>/day_YYYY_MM_DD.txt"
std::string dayLayoutPath(const std::string& saveDir, int year, int month, int day);
bool        hasSavedDayLayout(const std::string& saveDir, int year, int month, int day);

// Render a miniature preview of a saved DayLayout (including its daily photo
// and stickers) using our software rasterizer and Mat3 scaling.
Framebuffer renderDayThumbnail(const DayLayout& layout, int thumbW = 84, int thumbH = 48);
