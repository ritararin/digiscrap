#pragma once
#include <vector>
#include "core/mat3.h"          // Vec2, Polygon, Mat3
#include "core/framebuffer.h"   // Framebuffer, Color

// ---- Shape generators ----
Polygon makeStar(Vec2 c, double outer, double inner);
Polygon makeHeart(Vec2 c, double size);
Polygon makeSquare(Vec2 c, double side);
Polygon makeRegularPolygon(Vec2 c, double R, int sides);
Polygon makeDateStamp(Vec2 c, double w, double h, int notches = 8);

// ---- Operations ----
// (polygon transform already exists in core/mat3.h, so it is not declared here)
void    boundingBox(const Polygon& p, Vec2& minP, Vec2& maxP);
double  polygonArea(const Polygon& p);
Vec2    polygonCentroid(const Polygon& p);
bool    pointInPolygon(const Polygon& p, Vec2 pt);

// ---- Filling ----
struct Span { int y, x0, x1; };                   // fills x0 <= x < x1 on row y
std::vector<Span> polygonSpans(const Polygon& p); // scanline algorithm, no core dependency
void fillPolygon(Framebuffer& fb, const Polygon& p, Color c);