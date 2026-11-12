#pragma once
// ---------------------------------------------------------------------------
// day_storage.h — Multi-day layout model, plain-text serializer/loader,
// default page builder, and software thumbnail renderer (Option 5).
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
    std::string photoPath      = "demo";
    int         photoFilter    = 2;      // 2 = FILTER_SEPIA
    float       filterStrength = 1.0f;
    std::string dateText       = "05-10-2026";
    std::vector<SavedElement> elements;

    void sortByZ();
};

// Format "DD-MM-YYYY" for a date stamp.
std::string formatDateStamp(int year, int month, int day);

// Build a rich default scrapbook layout for (year, month, day).
DayLayout makeDefaultDayLayout(int year, int month, int day,
                               const std::string& photoPath = "demo");

// Plain-text serialization (in-memory string and file I/O).
std::string serializeDayLayout(const DayLayout& layout);
bool        deserializeDayLayout(const std::string& text, DayLayout& outLayout);

bool        saveDayLayout(const DayLayout& layout, const std::string& filePath);
bool        loadDayLayout(const std::string& filePath, DayLayout& outLayout);

// Standard path helper: "<saveDir>/day_YYYY_MM_DD.txt"
std::string dayLayoutPath(const std::string& saveDir, int year, int month, int day);
bool        hasSavedDayLayout(const std::string& saveDir, int year, int month, int day);

// Render a miniature preview of a saved DayLayout using our software rasterizer
// and Mat3 scaling, ready to upload as a calendar cell thumbnail texture.
Framebuffer renderDayThumbnail(const DayLayout& layout, int thumbW = 84, int thumbH = 44);
