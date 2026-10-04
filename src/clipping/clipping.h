#pragma once
#include "core/framebuffer.h"
#include "core/mat3.h"
// Cohen-Sutherland; returns false if fully outside
bool clipLine(double& x0, double& y0, double& x1, double& y1,
              double xmin, double ymin, double xmax, double ymax);
// Sutherland-Hodgman against a rectangle
Polygon clipPolygon(const Polygon&, double xmin, double ymin, double xmax, double ymax);
// Scanline fill
void fillPolygon(Framebuffer&, const Polygon&, Color);