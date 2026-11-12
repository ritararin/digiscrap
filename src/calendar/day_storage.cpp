#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "calendar/day_storage.h"
#include "polygons/polygons.h"
#include "curves/curves.h"
#include "lines/lines.h"
#include "colour/colour.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

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
            return makeStar({0, 0}, 50, 22);
        case ShapeKind::HEART:
            return makeHeart({0, 0}, 85);
        case ShapeKind::BEZIER_HEART:
            return makeBezierHeart({0, 0}, 85, 24);
        case ShapeKind::SQUARE:
            return makeSquare({0, 0}, 64);
        case ShapeKind::HEXAGON:
            return makeRegularPolygon({0, 0}, 40, 6);
        case ShapeKind::CIRCLE:
            return makeRegularPolygon({0, 0}, 36, 32);
        case ShapeKind::ELLIPSE: {
            Polygon p = makeRegularPolygon({0, 0}, 1.0, 32);
            return transformPolygon(p, Mat3::scale(42.0, 26.0));
        }
        case ShapeKind::CLOUD:
            return makeCloud({0, 0}, 120, 68, 12);
        case ShapeKind::CURVY_STAR:
            return makeCurvyStar({0, 0}, 38, 20, 5, 10);
        case ShapeKind::RIBBON:
            return makeRibbon({0, 0}, 120, 34, 10);
        case ShapeKind::TAPE:
            return {{-46, -13}, {46, -13}, {46, 13}, {-46, 13}};
        case ShapeKind::DATE_STAMP:
            return makeDateStamp({0, 0}, 220, 54, 10);
    }
    return makeStar({0, 0}, 50, 22);
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

DayLayout makeDefaultDayLayout(int year, int month, int day, const std::string& photoPath) {
    DayLayout L;
    L.year           = year;
    L.month          = month;
    L.day            = day;
    L.photoPath      = photoPath.empty() ? "demo" : photoPath;
    L.photoFilter    = 2;     // Sepia
    L.filterStrength = 1.0f;
    L.dateText       = formatDateStamp(year, month, day);

    // Palette matching main.cpp
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

    // Subtle per-day variation so different days feel unique
    double rotShift = ((day % 5) - 2) * 4.0;

    // 1. Translucent washi tape strips holding the photo corners
    L.elements.push_back(makeElem(ShapeKind::TAPE, {145, 92}, -32.0, {1.0, 1.0},
                                  tapeA, {180, 130, 50, 180}, 1));
    L.elements.push_back(makeElem(ShapeKind::TAPE, {455, 92},  32.0, {1.0, 1.0},
                                  tapeB, { 84, 118, 96, 180}, 2));

    // 2. Stickers (star, bezier heart, hexagon, square, circle, cloud, curvy star)
    L.elements.push_back(makeElem(ShapeKind::STAR, {505, 125}, 15.0 + rotShift, {1.15, 1.15},
                                  mustard, {150, 100, 30, 255}, 3));
    L.elements.push_back(makeElem(ShapeKind::BEZIER_HEART, {205, 360}, -18.0 + rotShift, {1.0, 1.0},
                                  terracotta, {120, 50, 35, 255}, 4));
    L.elements.push_back(makeElem(ShapeKind::HEXAGON, {625, 135}, 10.0, {1.0, 1.0},
                                  sage, dark, 5));
    L.elements.push_back(makeElem(ShapeKind::SQUARE, {565, 280}, 18.0 - rotShift, {1.0, 1.0},
                                  tint(honey, green, 0.55), {170, 120, 40, 255}, 6));
    L.elements.push_back(makeElem(ShapeKind::CIRCLE, {695, 235}, 0.0, {1.0, 1.0},
                                  green, deepGreen, 7));
    L.elements.push_back(makeElem(ShapeKind::CLOUD, {640, 345}, 0.0, {0.95, 0.95},
                                  white, brown, 8));
    L.elements.push_back(makeElem(ShapeKind::CURVY_STAR, {390, 475}, 12.0, {1.0, 1.0},
                                  honey, brown, 9));

    // 3. Date stamp sticker at bottom-right
    L.elements.push_back(makeElem(ShapeKind::DATE_STAMP, {610, 475}, -4.0, {1.0, 1.0},
                                  white, brown, 10));

    L.sortByZ();
    return L;
}

std::string serializeDayLayout(const DayLayout& layout) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(4);
    out << "DIGISCRAP_DAY_V1\n";
    out << "date " << layout.year << " " << layout.month << " " << layout.day << "\n";
    out << "photo " << (layout.photoPath.empty() ? "demo" : layout.photoPath) << "\n";
    out << "filter " << layout.photoFilter << " " << layout.filterStrength << "\n";
    out << "datetext " << (layout.dateText.empty()
                           ? formatDateStamp(layout.year, layout.month, layout.day)
                           : layout.dateText) << "\n";

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

    std::string line;
    std::getline(in, line); // consume remainder of header line
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
            parsed.photoPath = rest.empty() ? "demo" : rest;
        } else if (tag == "filter") {
            if (!(ls >> parsed.photoFilter >> parsed.filterStrength)) return false;
        } else if (tag == "datetext") {
            std::string rest;
            std::getline(ls >> std::ws, rest);
            if (!rest.empty() && rest.back() == '\r') rest.pop_back();
            parsed.dateText = rest;
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
    thumbW = std::max(8, thumbW);
    thumbH = std::max(8, thumbH);
    Framebuffer fb(thumbW, thumbH, {249, 228, 192, 255}); // warm cream paper

    // Scale from 800x600 page coordinates to thumbnail pixel coordinates
    const double sx = (double)thumbW / 800.0;
    const double sy = (double)thumbH / 600.0;
    const Mat3 pageToThumb = Mat3::scale(sx, sy);

    // Miniature polaroid photo card in top-left quadrant
    Polygon polaroid = transformPolygon({{120, 80}, {480, 80}, {480, 420}, {120, 420}}, pageToThumb);
    fillPolygon(fb, polaroid, {255, 252, 245, 255});
    Polygon photoArea = transformPolygon({{140, 100}, {460, 100}, {460, 360}, {140, 360}}, pageToThumb);
    Color photoCol = (layout.photoFilter == 1) ? Color{155, 155, 155, 255}
                   : (layout.photoFilter == 2) ? Color{198, 166, 122, 255}
                                               : Color{135, 175, 145, 255};
    fillPolygon(fb, photoArea, photoCol);

    // Draw every saved element in z-order using our software scanline fill + alphaBlend
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

    // Thin dark border around the thumbnail
    Color border{154, 106, 67, 255};
    drawLineBresenham(fb, 0, 0, thumbW - 1, 0, border);
    drawLineBresenham(fb, thumbW - 1, 0, thumbW - 1, thumbH - 1, border);
    drawLineBresenham(fb, thumbW - 1, thumbH - 1, 0, thumbH - 1, border);
    drawLineBresenham(fb, 0, thumbH - 1, 0, 0, border);

    return fb;
}
