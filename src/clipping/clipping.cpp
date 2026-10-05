#include "clipping/clipping.h"
#include "polygons/polygons.h"
#include <algorithm>
#include <cmath>
bool clipLine(double&, double&, double&, double&, double, double, double, double) { return true; } // TODO owner
Polygon clipPolygon(const Polygon& p, double, double, double, double) { return p; }                 // TODO owner
