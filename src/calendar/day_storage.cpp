#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define Polygon Win32GdiPolygon
#include <windows.h>
#include <commdlg.h>
#include <gdiplus.h>
#undef Polygon
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comdlg32.lib")
#endif
#include "calendar/day_storage.h"
#include "polygons/polygons.h"
#include "circles/circles.h"
#include "curves/curves.h"
#include "lines/lines.h"
#include "colour/colour.h"
#include "animation/lerp.h"
#include "textures/image_loader.h"
#include "textures/filter_mirror.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::string shapeKindToString(ShapeKind kind) {
    switch (kind) {
        case ShapeKind::STAR:         return "star";
        case ShapeKind::HEART:        return "heart";
        case ShapeKind::BEZIER_HEART: return "bezier_heart";
        case ShapeKind::SQUARE:       return "square";
        case ShapeKind::HEXAGON:      return "hexagon";
        case ShapeKind::CIRCLE:       return "circle";
        case ShapeKind::ELLIPSE:      return "ellipse";
        case ShapeKind::CLOUD:        return "cloud";
        case ShapeKind::CURVY_STAR:   return "curvy_star";
        case ShapeKind::RIBBON:       return "ribbon";
        case ShapeKind::TAPE:         return "tape";
        case ShapeKind::DATE_STAMP:   return "date_stamp";
    }
    return "star";
}

ShapeKind shapeKindFromString(const std::string& token, bool* ok) {
    if (ok) *ok = true;
    if (token == "star")         return ShapeKind::STAR;
    if (token == "heart")        return ShapeKind::HEART;
    if (token == "bezier_heart") return ShapeKind::BEZIER_HEART;
    if (token == "square")       return ShapeKind::SQUARE;
    if (token == "hexagon")      return ShapeKind::HEXAGON;
    if (token == "circle")       return ShapeKind::CIRCLE;
    if (token == "ellipse")      return ShapeKind::ELLIPSE;
    if (token == "cloud")        return ShapeKind::CLOUD;
    if (token == "curvy_star")   return ShapeKind::CURVY_STAR;
    if (token == "ribbon")       return ShapeKind::RIBBON;
    if (token == "tape")         return ShapeKind::TAPE;
    if (token == "date_stamp")   return ShapeKind::DATE_STAMP;
    if (ok) *ok = false;
    return ShapeKind::STAR;
}

Polygon buildShapePolygon(ShapeKind kind) {
    switch (kind) {
        case ShapeKind::STAR:
            return makeStar({0, 0}, 46, 20);
        case ShapeKind::HEART:
            return makeHeart({0, 0}, 78);
        case ShapeKind::BEZIER_HEART:
            return makeBezierHeart({0, 0}, 78, 24);
        case ShapeKind::SQUARE:
            return makeSquare({0, 0}, 58);
        case ShapeKind::HEXAGON:
            return makeRegularPolygon({0, 0}, 36, 6);
        case ShapeKind::CIRCLE:
            return makeRegularPolygon({0, 0}, 34, 32);
        case ShapeKind::ELLIPSE: {
            Polygon p = makeRegularPolygon({0, 0}, 1.0, 32);
            return transformPolygon(p, Mat3::scale(40.0, 25.0));
        }
        case ShapeKind::CLOUD:
            return makeCloud({0, 0}, 110, 62, 12);
        case ShapeKind::CURVY_STAR:
            return makeCurvyStar({0, 0}, 36, 19, 5, 10);
        case ShapeKind::RIBBON:
            return makeRibbon({0, 0}, 116, 32, 10);
        case ShapeKind::TAPE:
            return {{-46, -12}, {46, -12}, {46, 12}, {-46, 12}};
        case ShapeKind::DATE_STAMP:
            return makeDateStamp({0, 0}, 216, 52, 10);
    }
    return makeStar({0, 0}, 46, 20);
}

void DayLayout::sortByZ() {
    std::stable_sort(elements.begin(), elements.end(),
                     [](const SavedElement& a, const SavedElement& b) {
                         return a.zOrder < b.zOrder;
                     });
}

std::string formatDateStamp(int year, int month, int day) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d-%02d-%04d", day, month, year);
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// Daily Photo Generator (8 distinct illustrated scenes using CP1/CP2)
// ---------------------------------------------------------------------------

int photoThemeForDay(int year, int month, int day) {
    int idx = (day - 1 + month * 3 + year) % kNumPhotoThemes;
    return (idx < 0) ? (idx + kNumPhotoThemes) : idx;
}

int parsePhotoTheme(const std::string& photoPath, int fallbackDay) {
    if (photoPath.rfind("theme:", 0) == 0 && photoPath.size() > 6) {
        int v = std::atoi(photoPath.c_str() + 6);
        return ((v % kNumPhotoThemes) + kNumPhotoThemes) % kNumPhotoThemes;
    }
    return ((fallbackDay - 1) % kNumPhotoThemes + kNumPhotoThemes) % kNumPhotoThemes;
}

std::string makePhotoThemeSpec(int themeIndex) {
    int idx = ((themeIndex % kNumPhotoThemes) + kNumPhotoThemes) % kNumPhotoThemes;
    return "theme:" + std::to_string(idx);
}

std::string photoThemeCaption(int themeIndex) {
    static const char* kCaptions[kNumPhotoThemes] = {
        "SUNSET HIKE",
        "STARRY CAMP",
        "OCEAN BREEZE",
        "AUTUMN WALK",
        "CAFE MORNING",
        "FLOWER FIELD",
        "CITY LIGHTS",
        "RAINBOW DAY"
    };
    int idx = ((themeIndex % kNumPhotoThemes) + kNumPhotoThemes) % kNumPhotoThemes;
    return kCaptions[idx];
}

std::string defaultDayDescription(int year, int month, int day) {
    static const char* kNotes[kNumPhotoThemes] = {
        "GOLDEN SUNSET OVER THE RIDGE TRAIL WITH FRIENDS.",
        "CAMPFIRE BY THE LAKE UNDER A BRIGHT CRESCENT MOON.",
        "WARM OCEAN BREEZE AND SAILBOATS ALONG THE COAST.",
        "CRISP AUTUMN LEAVES ALONG OUR AFTERNOON PARK WALK.",
        "MORNING COFFEE AND WARM PASTRIES AT THE CORNER CAFE.",
        "SUNFLOWERS BLOOMING ACROSS THE GREEN HILLSIDE.",
        "EVENING SKYLINE GLOWING AS THE CITY LIGHTS CAME ON.",
        "A BRIGHT DOUBLE RAINBOW RIGHT AFTER THE RAIN CLEARED!"
    };
    int theme = photoThemeForDay(year, month, day);
    return kNotes[theme];
}

bool isCustomPhotoPath(const std::string& photoPath) {
    return !photoPath.empty() && photoPath != "demo" && photoPath.rfind("theme:", 0) != 0;
}

std::string customPhotoSavePath(const std::string& saveDir, int year, int month, int day) {
    std::string dir = saveDir.empty() ? "saves" : saveDir;
    char buf[96];
    std::snprintf(buf, sizeof(buf), "/photos/day_%04d_%02d_%02d.bmp", year, month, day);
    return dir + std::string(buf);
}

#ifdef _WIN32
static bool decodeImageWithGdiPlus(const std::string& filePath, Framebuffer& out) {
    static ULONG_PTR gdiplusToken = 0;
    if (gdiplusToken == 0) {
        Gdiplus::GdiplusStartupInput startupInput;
        if (Gdiplus::GdiplusStartup(&gdiplusToken, &startupInput, nullptr) != Gdiplus::Ok) {
            return false;
        }
    }

    int wlen = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0);
    if (wlen <= 0) {
        wlen = MultiByteToWideChar(CP_ACP, 0, filePath.c_str(), -1, nullptr, 0);
        if (wlen <= 0) return false;
    }
    std::wstring wpath((size_t)wlen, L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, wpath.data(), wlen) == 0) {
        MultiByteToWideChar(CP_ACP, 0, filePath.c_str(), -1, wpath.data(), wlen);
    }

    Gdiplus::Bitmap bmp(wpath.c_str());
    if (bmp.GetLastStatus() != Gdiplus::Ok) return false;

    int w = (int)bmp.GetWidth();
    int h = (int)bmp.GetHeight();
    if (w <= 0 || h <= 0) return false;

    out = Framebuffer(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Gdiplus::Color c;
            if (bmp.GetPixel(x, y, &c) == Gdiplus::Ok) {
                out.pixels[y * w + x] = {c.GetR(), c.GetG(), c.GetB(), c.GetA()};
            }
        }
    }
    return true;
}
#endif

bool importUserPhotoFile(const std::string& srcFilePath,
                         const std::string& destBmpPath,
                         Framebuffer& outFb,
                         int targetW, int targetH) {
    if (srcFilePath.empty()) return false;

    Framebuffer raw(1, 1);
    bool loadedOk = loadImage(srcFilePath, raw);
#ifdef _WIN32
    if (!loadedOk) {
        loadedOk = decodeImageWithGdiPlus(srcFilePath, raw);
    }
#endif
    if (!loadedOk || raw.width <= 0 || raw.height <= 0) {
        return false;
    }

    targetW = std::max(16, targetW);
    targetH = std::max(16, targetH);
    if (raw.width == targetW && raw.height == targetH) {
        outFb = raw;
    } else {
        outFb = resizeBilinear(raw, targetW, targetH);
    }

    if (!destBmpPath.empty()) {
        std::error_code ec;
        std::filesystem::path p(destBmpPath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path(), ec);
        }
        outFb.saveBMP(destBmpPath);
    }
    return true;
}

std::string openPhotoFileDialog() {
#ifdef _WIN32
    char fileBuf[MAX_PATH] = {0};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = GetActiveWindow();
    ofn.lpstrFilter =
        "Image Files (*.bmp;*.ppm;*.png;*.jpg;*.jpeg)\0*.bmp;*.ppm;*.png;*.jpg;*.jpeg\0"
        "BMP / PPM Images (*.bmp;*.ppm)\0*.bmp;*.ppm\0"
        "All Files (*.*)\0*.*\0";
    ofn.lpstrFile   = fileBuf;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = "Select a Photo for This Day's Scrapbook Page";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) {
        return std::string(fileBuf);
    }
#endif
    return "";
}

static void fillGradH(Framebuffer& fb, int y0, int y1, Color topCol, Color botCol) {
    y0 = std::max(0, y0);
    y1 = std::min(fb.height, y1);
    int span = std::max(1, y1 - y0 - 1);
    for (int y = y0; y < y1; ++y) {
        Color c = lerp(topCol, botCol, (double)(y - y0) / (double)span);
        for (int x = 0; x < fb.width; ++x) {
            fb.pixels[y * fb.width + x] = c;
        }
    }
}

Framebuffer makeDayPhoto(int themeIndex, int w, int h) {
    w = std::max(16, w);
    h = std::max(16, h);
    Framebuffer fb(w, h, {240, 230, 210, 255});
    int theme = ((themeIndex % kNumPhotoThemes) + kNumPhotoThemes) % kNumPhotoThemes;

    const double sx = w / 360.0;
    const double sy = h / 270.0;
    auto S = [&](double x, double y) -> Vec2 { return {x * sx, y * sy}; };
    auto R = [&](double r) -> int { return std::max(1, (int)std::lround(r * std::min(sx, sy))); };

    if (theme == 0) {
        // 0: SUNSET HIKE — Warm coral/amber gradient sky, glowing sun, layered mountain ridges, pine silhouettes
        fillGradH(fb, 0, (int)(h * 0.55), {118, 72, 128, 255}, {238, 132, 84, 255});
        fillGradH(fb, (int)(h * 0.55), h, {238, 132, 84, 255}, {252, 214, 118, 255});

        fillCircle(fb, (int)(255 * sx), (int)(105 * sy), R(44), {254, 198, 112, 255});
        fillCircle(fb, (int)(255 * sx), (int)(105 * sy), R(30), {255, 238, 165, 255});

        fillPolygon(fb, transformPolygon(makeCloud({95, 62}, 86, 38), Mat3::scale(sx, sy)), {248, 185, 155, 255});

        // Distant purple mountains
        fillPolygon(fb, {S(0, 185), S(65, 125), S(145, 162), S(225, 112), S(305, 155), S(360, 128), S(360, 270), S(0, 270)},
                    {112, 82, 116, 255});
        // Mid pine ridge
        fillPolygon(fb, {S(0, 210), S(85, 162), S(180, 192), S(275, 150), S(360, 185), S(360, 270), S(0, 270)},
                    {76, 104, 92, 255});
        // Foreground dark forest hill
        fillPolygon(fb, {S(0, 228), S(120, 196), S(245, 218), S(360, 194), S(360, 270), S(0, 270)},
                    {48, 76, 64, 255});
    } else if (theme == 1) {
        // 1: STARRY CAMP — Deep night sky, crescent moon, stars, lake reflection, glowing orange tent
        fillGradH(fb, 0, (int)(h * 0.66), {22, 30, 62, 255}, {52, 84, 118, 255});
        fillGradH(fb, (int)(h * 0.66), h, {34, 58, 86, 255}, {18, 32, 54, 255});

        // Moon
        fillCircle(fb, (int)(285 * sx), (int)(58 * sy), R(22), {252, 238, 175, 255});
        fillCircle(fb, (int)(293 * sx), (int)(52 * sy), R(18), {28, 40, 74, 255});

        // Twinkling stars
        const int starPts[8][2] = {{45,35},{95,60},{150,28},{210,52},{65,95},{175,82},{325,92},{130,55}};
        for (auto& sp : starPts) {
            fillPolygon(fb, transformPolygon(makeStar({(double)sp[0], (double)sp[1]}, 7, 3), Mat3::scale(sx, sy)),
                        {252, 235, 165, 255});
        }

        // Far shore mountains
        fillPolygon(fb, {S(0, 178), S(70, 122), S(155, 160), S(245, 115), S(360, 178)}, {30, 44, 68, 255});

        // Water shimmer lines
        for (int i = 0; i < 6; ++i) {
            int wy = (int)((190 + i * 11) * sy);
            drawLineBresenham(fb, (int)((260 - i * 6) * sx), wy, (int)((305 + i * 6) * sx), wy, {245, 220, 145, 255});
        }

        // Foreground shore & glowing A-frame tent
        fillPolygon(fb, {S(0, 212), S(230, 220), S(360, 248), S(360, 270), S(0, 270)}, {26, 42, 38, 255});
        fillPolygon(fb, {S(55, 235), S(95, 176), S(135, 235)}, {236, 138, 62, 255});
        fillPolygon(fb, {S(80, 235), S(95, 192), S(110, 235)}, {252, 218, 122, 255});
    } else if (theme == 2) {
        // 2: OCEAN BREEZE — Sunny coastal sky, turquoise sea, sandy beach, sailboat on horizon
        fillGradH(fb, 0, (int)(h * 0.52), {108, 182, 238, 255}, {202, 234, 250, 255});
        fillGradH(fb, (int)(h * 0.52), (int)(h * 0.80), {42, 132, 176, 255}, {92, 186, 196, 255});
        fillGradH(fb, (int)(h * 0.80), h, {242, 214, 162, 255}, {226, 190, 132, 255});

        fillCircle(fb, (int)(70 * sx), (int)(54 * sy), R(24), {255, 232, 132, 255});
        fillPolygon(fb, transformPolygon(makeCloud({255, 58}, 92, 42), Mat3::scale(sx, sy)), {255, 252, 245, 255});

        // Sailboat
        fillPolygon(fb, {S(175, 152), S(235, 152), S(222, 168), S(188, 168)}, {186, 76, 58, 255});
        fillPolygon(fb, {S(204, 92), S(204, 149), S(178, 149)}, {255, 252, 245, 255});
        fillPolygon(fb, {S(208, 98), S(232, 149), S(208, 149)}, {248, 238, 220, 255});

        // Gentle white foam line along the beach
        fillPolygon(fb, {S(0, 212), S(120, 204), S(250, 216), S(360, 208), S(360, 220), S(0, 222)},
                    {242, 250, 252, 255});
    } else if (theme == 3) {
        // 3: AUTUMN WALK — Golden hills, winding trail, terracotta/mustard/sage autumn trees
        fillGradH(fb, 0, (int)(h * 0.56), {178, 212, 228, 255}, {250, 226, 182, 255});
        fillGradH(fb, (int)(h * 0.56), h, {218, 166, 92, 255}, {176, 118, 68, 255});

        // Winding path
        fillPolygon(fb, {S(168, 152), S(192, 152), S(245, 270), S(115, 270)}, {238, 214, 176, 255});

        // Autumn trees (trunks + colourful canopies)
        struct Tree { double x, y, r; Color c; };
        const Tree trees[] = {
            {75, 138, 36, {196, 92, 70, 255}},
            {130, 128, 28, {228, 165, 69, 255}},
            {235, 132, 32, {214, 118, 62, 255}},
            {292, 142, 38, {173, 170, 134, 255}},
        };
        for (const auto& tr : trees) {
            fillPolygon(fb, {S(tr.x - 5, tr.y), S(tr.x + 5, tr.y), S(tr.x + 7, tr.y + 52), S(tr.x - 7, tr.y + 52)},
                        {108, 72, 48, 255});
            fillCircle(fb, (int)(tr.x * sx), (int)(tr.y * sy), R(tr.r), tr.c);
            fillCircle(fb, (int)((tr.x - 12) * sx), (int)((tr.y + 8) * sy), R(tr.r * 0.72), tr.c);
        }
    } else if (theme == 4) {
        // 4: CAFE MORNING — Warm wooden table, ceramic coffee cup with steam curls, saucer & pastry
        fillGradH(fb, 0, (int)(h * 0.58), {232, 212, 184, 255}, {208, 182, 148, 255});
        fillGradH(fb, (int)(h * 0.58), h, {154, 106, 67, 255}, {122, 80, 48, 255});

        // Saucer & coffee mug
        fillEllipse(fb, (int)(165 * sx), (int)(208 * sy), R(68), R(20), {248, 240, 226, 255});
        fillPolygon(fb, {S(118, 128), S(212, 128), S(198, 202), S(132, 202)}, {196, 92, 70, 255});
        fillEllipse(fb, (int)(165 * sx), (int)(128 * sy), R(47), R(14), {252, 244, 232, 255});
        fillEllipse(fb, (int)(165 * sx), (int)(129 * sy), R(40), R(10), {92, 56, 36, 255});
        // Mug handle
        drawCircleMidpoint(fb, (int)(214 * sx), (int)(162 * sy), R(18), {196, 92, 70, 255});

        // Rising steam lines
        for (int k = -1; k <= 1; ++k) {
            double bx = 165 + k * 18;
            Polygon steam = bezierCubic(S(bx, 108), S(bx - 12, 82), S(bx + 14, 56), S(bx - 4, 32), 20);
            for (size_t i = 0; i + 1 < steam.size(); ++i) {
                drawLineBresenham(fb, (int)steam[i].x, (int)steam[i].y,
                                  (int)steam[i + 1].x, (int)steam[i + 1].y, {252, 246, 236, 255});
            }
        }
        // Croissant on side plate
        fillEllipse(fb, (int)(282 * sx), (int)(216 * sy), R(42), R(14), {248, 240, 226, 255});
        fillEllipse(fb, (int)(282 * sx), (int)(208 * sy), R(26), R(12), {228, 165, 69, 255});
    } else if (theme == 5) {
        // 5: FLOWER FIELD — Blue sky, clouds, green meadow, golden sunflowers in foreground
        fillGradH(fb, 0, (int)(h * 0.54), {122, 192, 242, 255}, {214, 238, 250, 255});
        fillPolygon(fb, transformPolygon(makeCloud({110, 56}, 88, 40), Mat3::scale(sx, sy)), {255, 252, 245, 255});
        fillPolygon(fb, transformPolygon(makeCloud({265, 70}, 76, 34), Mat3::scale(sx, sy)), {255, 252, 245, 255});

        fillPolygon(fb, {S(0, 155), S(110, 132), S(240, 150), S(360, 128), S(360, 270), S(0, 270)},
                    {110, 162, 104, 255});
        fillPolygon(fb, {S(0, 192), S(160, 172), S(360, 188), S(360, 270), S(0, 270)},
                    {84, 134, 86, 255});

        // Sunflowers
        const double fx[4] = {72, 148, 224, 296};
        const double fy[4] = {188, 174, 192, 178};
        for (int i = 0; i < 4; ++i) {
            fillPolygon(fb, {S(fx[i] - 3, fy[i]), S(fx[i] + 3, fy[i]), S(fx[i] + 3, 270), S(fx[i] - 3, 270)},
                        {62, 106, 64, 255});
            Polygon petals = makeCurvyStar({fx[i], fy[i]}, 28, 15, 8, 6);
            fillPolygon(fb, transformPolygon(petals, Mat3::scale(sx, sy)), {244, 186, 52, 255});
            fillCircle(fb, (int)(fx[i] * sx), (int)(fy[i] * sy), R(11), {96, 62, 38, 255});
        }
    } else if (theme == 6) {
        // 6: CITY LIGHTS — Violet-to-peach dusk sky, city skyline silhouette with lit windows, water reflection
        fillGradH(fb, 0, (int)(h * 0.42), {56, 46, 96, 255}, {168, 88, 114, 255});
        fillGradH(fb, (int)(h * 0.42), (int)(h * 0.76), {168, 88, 114, 255}, {244, 174, 108, 255});
        fillGradH(fb, (int)(h * 0.76), h, {44, 38, 68, 255}, {26, 22, 44, 255});

        const int bldgs[7][3] = {
            {18, 44, 110}, {66, 52, 78}, {122, 46, 98}, {172, 58, 64},
            {234, 42, 104}, {280, 48, 86}, {330, 30, 118}
        };
        for (auto& b : bldgs) {
            double bx = b[0], bw = b[1], topY = b[2];
            fillPolygon(fb, {S(bx, topY), S(bx + bw, topY), S(bx + bw, 206), S(bx, 206)}, {38, 34, 58, 255});
            for (double wy = topY + 10; wy < 196; wy += 16) {
                for (double wx = bx + 8; wx + 8 < bx + bw; wx += 14) {
                    fillPolygon(fb, {S(wx, wy), S(wx + 6, wy), S(wx + 6, wy + 8), S(wx, wy + 8)},
                                {252, 218, 122, 255});
                }
            }
        }
    } else {
        // 7: RAINBOW DAY — Sky clearing after rain, multi-band rainbow arch, green valley & clouds
        fillGradH(fb, 0, (int)(h * 0.65), {136, 188, 232, 255}, {218, 238, 248, 255});
        const Color bands[5] = {
            {226, 98, 86, 255}, {242, 168, 72, 255}, {252, 222, 106, 255},
            {112, 182, 126, 255}, {98, 162, 224, 255}
        };
        for (int b = 0; b < 5; ++b) {
            fillCircle(fb, (int)(180 * sx), (int)(195 * sy), R(128 - b * 9), bands[b]);
        }
        fillCircle(fb, (int)(180 * sx), (int)(195 * sy), R(82), {196, 226, 242, 255});

        fillPolygon(fb, {S(0, 195), S(115, 162), S(245, 182), S(360, 156), S(360, 270), S(0, 270)},
                    {96, 148, 98, 255});
        fillPolygon(fb, transformPolygon(makeCloud({75, 178}, 86, 40), Mat3::scale(sx, sy)), {255, 252, 245, 255});
        fillPolygon(fb, transformPolygon(makeCloud({285, 168}, 86, 40), Mat3::scale(sx, sy)), {255, 252, 245, 255});
    }

    return fb;
}

Framebuffer resolveDayPhoto(const DayLayout& layout, int w, int h) {
    if (isCustomPhotoPath(layout.photoPath)) {
        Framebuffer loaded(1, 1);
        if (loadImage(layout.photoPath, loaded)) {
            if (loaded.width == w && loaded.height == h) return loaded;
            return resizeBilinear(loaded, w, h);
        }
#ifdef _WIN32
        if (decodeImageWithGdiPlus(layout.photoPath, loaded)) {
            if (loaded.width == w && loaded.height == h) return loaded;
            return resizeBilinear(loaded, w, h);
        }
#endif
    }
    int theme = parsePhotoTheme(layout.photoPath, layout.day);
    return makeDayPhoto(theme, w, h);
}

static SavedElement makeElem(ShapeKind kind, Vec2 pos, double rotDeg, Vec2 scale,
                             Color fill, Color edge, int z) {
    SavedElement e;
    e.kind           = kind;
    e.xform.pivot    = {0, 0};
    e.xform.position = pos;
    e.xform.rotation = rotDeg;
    e.xform.scale    = scale;
    e.fill           = fill;
    e.edge           = edge;
    e.zOrder         = z;
    return e;
}

DayLayout makeDefaultDayLayout(int year, int month, int day, const std::string& customPhotoPath) {
    DayLayout L;
    L.year  = year;
    L.month = month;
    L.day   = day;

    int theme = photoThemeForDay(year, month, day);
    if (customPhotoPath.empty() || customPhotoPath == "demo") {
        L.photoPath = makePhotoThemeSpec(theme);
    } else {
        L.photoPath = customPhotoPath;
    }

    // Subtle warm sepia on Day 5 (matching the CP3 prototype spec) and clean/vivid on other days
    L.photoFilter    = (day == 5) ? 2 : 0;
    L.filterStrength = (day == 5) ? 0.45f : 1.0f;
    L.dateText       = formatDateStamp(year, month, day);
    L.description    = defaultDayDescription(year, month, day);

    const Color mustard   {228, 165,  69, 255};
    const Color honey     {249, 205, 125, 255};
    const Color green     {110, 145, 121, 255};
    const Color deepGreen { 84, 118,  96, 255};
    const Color sage      {173, 170, 134, 255};
    const Color terracotta{196,  92,  70, 255};
    const Color white     {255, 252, 245, 255};
    const Color brown     {154, 106,  67, 255};
    const Color dark      { 77,  74,  63, 255};
    const Color tapeA     {228, 165,  69, 195};
    const Color tapeB     {110, 145, 121, 195};

    double rotShift = ((day % 5) - 2) * 4.0;
    double yShift   = ((day % 3) - 1) * 8.0;

    // 1. Translucent washi tape strips holding the Polaroid corners
    L.elements.push_back(makeElem(ShapeKind::TAPE, {136, 92}, -30.0, {1.0, 1.0},
                                  tapeA, {180, 130, 50, 180}, 1));
    L.elements.push_back(makeElem(ShapeKind::TAPE, {438, 92},  30.0, {1.0, 1.0},
                                  tapeB, { 84, 118, 96, 180}, 2));

    // 2. Stickers arranged on the right journal card and Polaroid corner
    L.elements.push_back(makeElem(ShapeKind::STAR, {525, 132 + yShift}, 14.0 + rotShift, {1.05, 1.05},
                                  mustard, {150, 100, 30, 255}, 3));
    L.elements.push_back(makeElem(ShapeKind::BEZIER_HEART, {180, 355}, -16.0 + rotShift, {0.90, 0.90},
                                  terracotta, {120, 50, 35, 255}, 4));
    L.elements.push_back(makeElem(ShapeKind::HEXAGON, {665, 136 - yShift}, 10.0, {1.0, 1.0},
                                  sage, dark, 5));
    L.elements.push_back(makeElem(ShapeKind::SQUARE, {555, 236}, 16.0 - rotShift, {0.92, 0.92},
                                  tint(honey, green, 0.55), {170, 120, 40, 255}, 6));
    L.elements.push_back(makeElem(ShapeKind::CIRCLE, {690, 234}, 0.0, {0.95, 0.95},
                                  green, deepGreen, 7));
    L.elements.push_back(makeElem(ShapeKind::CLOUD, {625, 318}, 0.0, {0.88, 0.88},
                                  white, brown, 8));
    L.elements.push_back(makeElem(ShapeKind::CURVY_STAR, {505, 378}, 12.0, {0.90, 0.90},
                                  honey, brown, 9));

    // 3. Date stamp sticker above the sentence description box
    L.elements.push_back(makeElem(ShapeKind::DATE_STAMP, {632, 384}, -2.5, {0.94, 0.94},
                                  white, brown, 10));

    L.sortByZ();
    return L;
}

std::string serializeDayLayout(const DayLayout& layout) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(4);
    out << "DIGISCRAP_DAY_V1\n";
    out << "date " << layout.year << " " << layout.month << " " << layout.day << "\n";
    out << "photo " << (layout.photoPath.empty() ? "theme:0" : layout.photoPath) << "\n";
    out << "filter " << layout.photoFilter << " " << layout.filterStrength << "\n";
    out << "datetext " << (layout.dateText.empty()
                           ? formatDateStamp(layout.year, layout.month, layout.day)
                           : layout.dateText) << "\n";
    out << "note " << layout.description << "\n";

    for (const auto& e : layout.elements) {
        out << "element " << shapeKindToString(e.kind)
            << " " << e.xform.position.x << " " << e.xform.position.y
            << " " << e.xform.rotation
            << " " << e.xform.scale.x    << " " << e.xform.scale.y
            << " " << e.xform.pivot.x    << " " << e.xform.pivot.y
            << " " << (int)e.fill.r << " " << (int)e.fill.g << " " << (int)e.fill.b << " " << (int)e.fill.a
            << " " << (int)e.edge.r << " " << (int)e.edge.g << " " << (int)e.edge.b << " " << (int)e.edge.a
            << " " << e.zOrder << "\n";
    }
    return out.str();
}

bool deserializeDayLayout(const std::string& text, DayLayout& outLayout) {
    std::istringstream in(text);
    std::string header;
    if (!(in >> header) || header != "DIGISCRAP_DAY_V1") {
        return false;
    }

    DayLayout parsed;
    parsed.elements.clear();
    parsed.description.clear();
    bool hasNoteTag = false;

    std::string line;
    std::getline(in, line);
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        std::string tag;
        if (!(ls >> tag)) continue;

        if (tag == "date") {
            if (!(ls >> parsed.year >> parsed.month >> parsed.day)) return false;
        } else if (tag == "photo") {
            std::string rest;
            std::getline(ls >> std::ws, rest);
            if (!rest.empty() && rest.back() == '\r') rest.pop_back();
            parsed.photoPath = rest.empty() ? "theme:0" : rest;
        } else if (tag == "filter") {
            if (!(ls >> parsed.photoFilter >> parsed.filterStrength)) return false;
        } else if (tag == "datetext") {
            std::string rest;
            std::getline(ls >> std::ws, rest);
            if (!rest.empty() && rest.back() == '\r') rest.pop_back();
            parsed.dateText = rest;
        } else if (tag == "note") {
            hasNoteTag = true;
            std::string rest;
            std::getline(ls >> std::ws, rest);
            if (!rest.empty() && rest.back() == '\r') rest.pop_back();
            parsed.description = rest;
        } else if (tag == "element") {
            std::string kindStr;
            SavedElement e;
            int fr, fg, fb, fa, er, eg, eb, ea;
            if (!(ls >> kindStr
                     >> e.xform.position.x >> e.xform.position.y
                     >> e.xform.rotation
                     >> e.xform.scale.x >> e.xform.scale.y
                     >> e.xform.pivot.x >> e.xform.pivot.y
                     >> fr >> fg >> fb >> fa
                     >> er >> eg >> eb >> ea
                     >> e.zOrder)) {
                return false;
            }
            bool kindOk = false;
            e.kind = shapeKindFromString(kindStr, &kindOk);
            if (!kindOk) return false;
            e.fill = {(uint8_t)std::clamp(fr, 0, 255),
                      (uint8_t)std::clamp(fg, 0, 255),
                      (uint8_t)std::clamp(fb, 0, 255),
                      (uint8_t)std::clamp(fa, 0, 255)};
            e.edge = {(uint8_t)std::clamp(er, 0, 255),
                      (uint8_t)std::clamp(eg, 0, 255),
                      (uint8_t)std::clamp(eb, 0, 255),
                      (uint8_t)std::clamp(ea, 0, 255)};
            parsed.elements.push_back(e);
        }
    }

    if (!hasNoteTag) {
        parsed.description = defaultDayDescription(parsed.year, parsed.month, parsed.day);
    }

    parsed.sortByZ();
    outLayout = std::move(parsed);
    return true;
}

bool saveDayLayout(const DayLayout& layout, const std::string& filePath) {
    std::error_code ec;
    std::filesystem::path p(filePath);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    std::ofstream out(filePath, std::ios::out | std::ios::trunc);
    if (!out) return false;
    out << serializeDayLayout(layout);
    return out.good();
}

bool loadDayLayout(const std::string& filePath, DayLayout& outLayout) {
    std::ifstream in(filePath);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    return deserializeDayLayout(ss.str(), outLayout);
}

std::string dayLayoutPath(const std::string& saveDir, int year, int month, int day) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "/day_%04d_%02d_%02d.txt", year, month, day);
    return saveDir + std::string(buf);
}

bool hasSavedDayLayout(const std::string& saveDir, int year, int month, int day) {
    std::error_code ec;
    return std::filesystem::exists(dayLayoutPath(saveDir, year, month, day), ec);
}

Framebuffer renderDayThumbnail(const DayLayout& layout, int thumbW, int thumbH) {
    thumbW = std::max(12, thumbW);
    thumbH = std::max(12, thumbH);
    Framebuffer fb(thumbW, thumbH, {255, 252, 245, 255}); // crisp Polaroid white frame

    // Render the day's actual distinct photograph inside the left/main portion of the thumbnail
    int px0 = 2, py0 = 2;
    int pw  = std::max(6, (int)(thumbW * 0.56));
    int ph  = std::max(6, thumbH - 6);
    Framebuffer miniPhoto = resolveDayPhoto(layout, pw, ph);

    FilterParams fp;
    fp.kind     = (FilterKind)std::clamp(layout.photoFilter, 0, (int)FILTER_COUNT - 1);
    fp.strength = layout.filterStrength;
    for (int y = 0; y < ph; ++y) {
        for (int x = 0; x < pw; ++x) {
            Color c = miniPhoto.pixels[y * pw + x];
            if (fp.kind != FILTER_NONE && fp.strength > 0.0f) {
                c = applyFilterMirror(c, fp);
            }
            fb.setPixel(px0 + x, py0 + y, c);
        }
    }

    // Soft cream right-hand journal area for miniature stickers
    for (int y = 1; y < thumbH - 1; ++y) {
        for (int x = px0 + pw + 2; x < thumbW - 1; ++x) {
            fb.setPixel(x, y, {249, 228, 192, 255});
        }
    }

    // Scale from 800x600 page coordinates to thumbnail pixel coordinates
    const double sx = (double)thumbW / 800.0;
    const double sy = (double)thumbH / 600.0;
    const Mat3 pageToThumb = Mat3::scale(sx, sy);

    DayLayout sorted = layout;
    sorted.sortByZ();
    for (const auto& elem : sorted.elements) {
        Polygon base = buildShapePolygon(elem.kind);
        Polygon wp   = transformPolygon(base, pageToThumb * elem.xform.toMat3());
        if (elem.fill.a == 255) {
            fillPolygon(fb, wp, elem.fill);
        } else {
            for (const Span& sp : polygonSpans(wp)) {
                if (sp.y < 0 || sp.y >= fb.height) continue;
                int x0 = std::max(0, sp.x0);
                int x1 = std::min(fb.width, sp.x1);
                for (int x = x0; x < x1; ++x) {
                    Color dst = fb.pixels[sp.y * fb.width + x];
                    fb.pixels[sp.y * fb.width + x] = alphaBlend(elem.fill, dst);
                }
            }
        }
    }

    // Crisp warm frame border
    Color border{154, 106, 67, 255};
    drawLineBresenham(fb, 0, 0, thumbW - 1, 0, border);
    drawLineBresenham(fb, thumbW - 1, 0, thumbW - 1, thumbH - 1, border);
    drawLineBresenham(fb, thumbW - 1, thumbH - 1, 0, thumbH - 1, border);
    drawLineBresenham(fb, 0, thumbH - 1, 0, 0, border);

    return fb;
}
