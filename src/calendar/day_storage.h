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
    std::string photoPath      = "none"; // Path to user's uploaded photo file ("none" if no photo uploaded yet)
    int         photoFilter    = 0;      // 0 = FILTER_NONE (F cycles sepia/grayscale/etc.)
    float       filterStrength = 1.0f;
    std::string dateText       = "05-10-2026";
    std::string description    = "";     // User's daily journal text
    std::vector<SavedElement> elements;

    void sortByZ();
};

// Format "DD-MM-YYYY" for a date stamp.
std::string formatDateStamp(int year, int month, int day);

// Photo helpers — checks whether a day has a real user-uploaded image file.
bool        isCustomPhotoPath(const std::string& photoPath);
bool        hasActualPhotoFile(const DayLayout& layout);
Framebuffer resolveDayPhoto(const DayLayout& layout, int w = 360, int h = 270);

// Import a user image file (.bmp, .ppm, .png, .jpg, .jpeg) into `outFb` (resized
// to targetW x targetH using our resizeBilinear) and save a 24-bit BMP copy at
// `destBmpPath` so future loads and thumbnails use our own BMP loader.
std::string customPhotoSavePath(const std::string& saveDir, int year, int month, int day);
bool        importUserPhotoFile(const std::string& srcFilePath,
                                const std::string& destBmpPath,
                                Framebuffer& outFb,
                                int targetW = 360, int targetH = 270);

// Open a native OS file picker dialog to choose a photo file (returns "" if cancelled).
std::string openPhotoFileDialog();

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
