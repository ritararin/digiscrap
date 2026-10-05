#include "clipping/clipping.h"
#include "polygons/polygons.h"
#include <algorithm>
#include <cmath>

namespace {
enum Region { Left = 1, Right = 2, Below = 4, Above = 8 };

int regionCode(double x, double y, double xmin, double ymin, double xmax, double ymax) {
	int code = 0;
	if (x < xmin) code |= Left;
	if (x > xmax) code |= Right;
	if (y < ymin) code |= Below;
	if (y > ymax) code |= Above;
	return code;
}
}

bool clipLine(double& x0, double& y0, double& x1, double& y1,
			  double xmin, double ymin, double xmax, double ymax) {
	if (xmin > xmax || ymin > ymax) return false;
	double ax = x0, ay = y0, bx = x1, by = y1;
	int codeA = regionCode(ax, ay, xmin, ymin, xmax, ymax);
	int codeB = regionCode(bx, by, xmin, ymin, xmax, ymax);
	for (;;) {
		if ((codeA | codeB) == 0) {
			x0 = ax; y0 = ay; x1 = bx; y1 = by;
			return true;
		}
		// A shared outside bit puts the whole segment beyond that edge.
		if ((codeA & codeB) != 0) return false;

		int outside = codeA != 0 ? codeA : codeB;
		double ix, iy;
		// The trivial rejection above excludes parallel lines beyond this edge,
		// so the denominator for the selected intersection cannot be zero.
		if (outside & Above) {
			iy = ymax;
			ix = ax + (bx - ax) * (ymax - ay) / (by - ay);
		} else if (outside & Below) {
			iy = ymin;
			ix = ax + (bx - ax) * (ymin - ay) / (by - ay);
		} else if (outside & Right) {
			ix = xmax;
			iy = ay + (by - ay) * (xmax - ax) / (bx - ax);
		} else {
			ix = xmin;
			iy = ay + (by - ay) * (xmin - ax) / (bx - ax);
		}
		if (outside == codeA) {
			ax = ix; ay = iy;
			codeA = regionCode(ax, ay, xmin, ymin, xmax, ymax);
		} else {
			bx = ix; by = iy;
			codeB = regionCode(bx, by, xmin, ymin, xmax, ymax);
		}
	}
}

bool clipLineLiangBarsky(double& x0, double& y0, double& x1, double& y1,
						double xmin, double ymin, double xmax, double ymax) {
	if (xmin > xmax || ymin > ymax) return false;
	double dx = x1 - x0, dy = y1 - y0;
	double t0 = 0.0, t1 = 1.0;
	const double p[4] = {-dx, dx, -dy, dy};
	const double q[4] = {x0 - xmin, xmax - x0, y0 - ymin, ymax - y0};
	// Each edge requires p*t <= q. Entering edges tighten the lower bound;
	// leaving edges tighten the upper bound of the visible parameter interval.
	for (int edge = 0; edge < 4; edge++) {
		if (p[edge] == 0.0) {
			if (q[edge] < 0.0) return false;
		} else {
			double ratio = q[edge] / p[edge];
			if (p[edge] < 0.0) t0 = std::max(t0, ratio);
			else t1 = std::min(t1, ratio);
			if (t0 > t1) return false;
		}
	}
	double ax = x0, ay = y0;
	x0 = ax + t0 * dx; y0 = ay + t0 * dy;
	x1 = ax + t1 * dx; y1 = ay + t1 * dy;
	return true;
}

Polygon clipPolygon(const Polygon& polygon, double xmin, double ymin, double xmax, double ymax) {
	if (polygon.size() < 3 || xmin > xmax || ymin > ymax) return {};
	Polygon output = polygon;
	// Clip against left, right, below and above in turn. Each pass walks the
	// closed boundary, including the edge from the final vertex to the first.
	for (int edge = 0; edge < 4; edge++) {
		Polygon input = std::move(output);
		output.clear();
		if (input.empty()) break;
		auto inside = [&](Vec2 vertex) {
			switch (edge) {
				case 0: return vertex.x >= xmin;
				case 1: return vertex.x <= xmax;
				case 2: return vertex.y >= ymin;
				default: return vertex.y <= ymax;
			}
		};
		auto intersection = [&](Vec2 previous, Vec2 current) -> Vec2 {
			// Called only when inside/outside changes: the segment crosses
			// this boundary, so its corresponding coordinate changes too.
			if (edge < 2) {
				double boundary = edge == 0 ? xmin : xmax;
				double parameter = (boundary - previous.x) / (current.x - previous.x);
				return {boundary, previous.y + parameter * (current.y - previous.y)};
			}
			double boundary = edge == 2 ? ymin : ymax;
			double parameter = (boundary - previous.y) / (current.y - previous.y);
			return {previous.x + parameter * (current.x - previous.x), boundary};
		};
		Vec2 previous = input.back();
		bool previousInside = inside(previous);
		for (Vec2 current : input) {
			bool currentInside = inside(current);
			// in->out emits only the intersection; out->in emits it then
			// current. in->in keeps current; out->out emits nothing.
			if (previousInside != currentInside)
				output.push_back(intersection(previous, current));
			if (currentInside) output.push_back(current);
			previous = current;
			previousInside = currentInside;
		}
	}
	return output;
}
