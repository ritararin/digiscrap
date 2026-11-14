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
#include <cstdlib>
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

bool isCustomPhotoPath(const std::string& photoPath) {
    return !photoPath.empty() &&
           photoPath != "none" &&
           photoPath != "demo" &&
           photoPath.rfind("theme:", 0) != 0;
}

bool hasActualPhotoFile(const DayLayout& layout) {
    if (!isCustomPhotoPath(layout.photoPath)) return false;
    std::error_code ec;
    return std::filesystem::exists(layout.photoPath, ec);
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

#ifndef _WIN32
namespace {

// Wrap text in single quotes so /bin/sh treats it as ONE word (a ' inside becomes '\'').
[[maybe_unused]] std::string shellQuote(const std::string& text) {
    std::string out = "'";
    for (char c : text) {
        if (c == '\'') out += "'\\''";
        else            out += c;
    }
    out += "'";
    return out;
}

// Run a shell command and return what it printed, without the trailing newline.
std::string runAndCapture(const std::string& command) {
    std::string out;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return out;
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe)) out += buf;
    pclose(pipe);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

// Our own loaders only read BMP and PPM. On macOS the built-in `sips` tool converts any
// other picture (PNG, JPG, HEIC ...) to a temporary BMP that loadBMP can then read.
bool decodeImageWithSystemTool(const std::string& srcFilePath, Framebuffer& out) {
#ifdef __APPLE__
    std::error_code ec;
    const std::filesystem::path tmp = std::filesystem::temp_directory_path(ec) / "digiscrap_import.bmp";
    if (ec) return false;
    std::filesystem::remove(tmp, ec);
    const std::string cmd = "sips -s format bmp " + shellQuote(srcFilePath) +
                            " --out " + shellQuote(tmp.string()) + " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) return false;
    const bool ok = loadBMP(tmp.string(), out);
    std::filesystem::remove(tmp, ec);
    return ok;
#else
    (void)srcFilePath;
    (void)out;
    return false;
#endif
}

}  // namespace
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
#else
    if (!loadedOk) {
        loadedOk = decodeImageWithSystemTool(srcFilePath, raw);
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
    return "";
#elif defined(__APPLE__)
    // macOS: the system "choose file" dialog (cancelling prints nothing, so we return "").
    return runAndCapture(
        "osascript -e 'POSIX path of (choose file with prompt "
        "\"Select a photo for this day\" of type {\"public.image\"})' 2>/dev/null");
#else
    // Linux: use zenity when it is installed; otherwise drag a file onto the window.
    return runAndCapture("zenity --file-selection --title='Select a photo' 2>/dev/null");
#endif
}

Framebuffer resolveDayPhoto(const DayLayout& layout, int w, int h) {
    w = std::max(16, w);
    h = std::max(16, h);
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
    // No photo uploaded yet: return a clean warm linen photo placeholder frame
    Framebuffer emptySlot(w, h, {242, 232, 214, 255});
    Color border{196, 174, 144, 255};
    drawLineBresenham(emptySlot, 0, 0, w - 1, 0, border);
    drawLineBresenham(emptySlot, w - 1, 0, w - 1, h - 1, border);
    drawLineBresenham(emptySlot, w - 1, h - 1, 0, h - 1, border);
    drawLineBresenham(emptySlot, 0, h - 1, 0, 0, border);
    return emptySlot;
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

    if (isCustomPhotoPath(customPhotoPath)) {
        L.photoPath = customPhotoPath;
    } else {
        L.photoPath = "none";
    }

    L.photoFilter    = 0;
    L.filterStrength = 1.0f;
    L.dateText       = formatDateStamp(year, month, day);
    L.description    = "";

    const Color white {255, 252, 245, 255};
    const Color brown {154, 106,  67, 255};
    const Color tapeA {228, 165,  69, 195};
    const Color tapeB {110, 145, 121, 195};

    // 1. Translucent washi tape strips holding the Polaroid corners
    L.elements.push_back(makeElem(ShapeKind::TAPE, {136, 92}, -30.0, {1.0, 1.0},
                                  tapeA, {180, 130, 50, 180}, 1));
    L.elements.push_back(makeElem(ShapeKind::TAPE, {438, 92},  30.0, {1.0, 1.0},
                                  tapeB, { 84, 118, 96, 180}, 2));

    // 2. Date stamp sticker near bottom-right of the Polaroid / journal corner
    L.elements.push_back(makeElem(ShapeKind::DATE_STAMP, {612, 396}, -2.0, {0.92, 0.92},
                                  white, brown, 3));

    L.sortByZ();
    return L;
}

std::string serializeDayLayout(const DayLayout& layout) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(4);
    out << "DIGISCRAP_DAY_V1\n";
    out << "date " << layout.year << " " << layout.month << " " << layout.day << "\n";
    out << "photo " << (isCustomPhotoPath(layout.photoPath) ? layout.photoPath : "none") << "\n";
    out << "filter " << layout.photoFilter << " " << layout.filterStrength << "\n";
    out << "datetext " << (layout.dateText.empty()
                           ? formatDateStamp(layout.year, layout.month, layout.day)
                           : layout.dateText) << "\n";
    std::string encodedNote = layout.description;
    for (char& ch : encodedNote) {
        if (ch == '\n' || ch == '\r') ch = '|';
    }
    out << "note " << encodedNote << "\n";

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
            parsed.photoPath = isCustomPhotoPath(rest) ? rest : "none";
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
            for (char& ch : rest) {
                if (ch == '|') ch = '\n';
            }
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

    (void)hasNoteTag;
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
