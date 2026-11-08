// paths.h - motion paths for the Digital Scrapbook animations (Checkpoint 2, Option 5).
//
// A motion path is a curve a sticker travels along. A path takes a value t in [0,1] and returns
// a point. Combine it with an easing: position = path(ease(t)).
//
//   * Bezier paths      - a smooth curve shaped by control points (the curve does NOT pass
//                         through the middle control points; they pull on it).
//   * Catmull-Rom paths - a smooth curve that DOES pass through every waypoint you give it.
//   * Arc-length table  - makes the speed along any path even (see below).
//   * Page curl         - the curved fold edge used by the page-turn transition.
//
// Point2 is a tiny self-contained type so this module has no dependencies. If the team's Vec2
// is available, convert at the call site (Point2{v.x, v.y}).
#pragma once
#include <functional>
#include <vector>

namespace motion {

struct Point2 {
    double x = 0.0;
    double y = 0.0;
};

// ---- Bezier curves --------------------------------------------------------------------------
// Quadratic (3 control points) and cubic (4 control points), t in [0,1].
Point2 quadBezier(Point2 p0, Point2 p1, Point2 p2, double t);
Point2 cubicBezier(Point2 p0, Point2 p1, Point2 p2, Point2 p3, double t);
// Direction of travel (derivative) of the cubic at t. Use atan2(dy, dx) to turn a sticker so it
// faces along its path.
Point2 cubicBezierTangent(Point2 p0, Point2 p1, Point2 p2, Point2 p3, double t);

// ---- Catmull-Rom spline through waypoints ---------------------------------------------------
// Needs at least 2 waypoints. The path starts at the first waypoint (t=0), passes through every
// waypoint in order, and ends at the last (t=1). Each segment gets an equal share of t, so with
// 5 waypoints, t = 0, 0.25, 0.5, 0.75, 1 land exactly on waypoints 0..4.
class CatmullRomPath {
public:
    explicit CatmullRomPath(std::vector<Point2> waypoints);
    Point2 at(double t) const;
    const std::vector<Point2>& waypoints() const { return pts_; }

private:
    std::vector<Point2> pts_;
};

// ---- Arc-length parameterisation (optional, keeps speed even) -------------------------------
// On a curve, equal steps in t do NOT give equal distances: the sticker speeds up where the
// control points are spread out and slows down where they bunch up. This table measures the
// curve's length by sampling it, so you can ask "where is the point 30% of the way ALONG the
// curve?" and move at constant speed.
class ArcLengthTable {
public:
    // curve: any function t -> point. samples: more = more accurate (200 is plenty).
    ArcLengthTable(std::function<Point2(double)> curve, int samples = 200);
    double totalLength() const { return lengths_.back(); }
    // Curve parameter t whose distance from the start is u * totalLength, u in [0,1].
    double tAtFraction(double u) const;

private:
    std::vector<double> lengths_;  // lengths_[i] = distance from start to sample i
};

// ---- Page curl for the page-turn transition -------------------------------------------------
// The fold edge of a page being turned, as a polyline from the top of the page to the bottom.
// t = 0: the edge is on the far right of the page (nothing turned).
// t = 1: the edge is on the far left (page fully turned).
// The edge is a cubic Bezier that bows to the left in the middle, so the page looks curled
// rather than cut with a straight wipe. samples >= 2 is the number of polyline points.
std::vector<Point2> pageCurlEdge(double t, double pageWidth, double pageHeight, int samples = 40);

// The part of the page that is still showing (left of the fold): the edge polyline closed with
// the top-left and bottom-left corners. This is the polygon to hand to Sutherland-Hodgman or to
// fill when you want the old page on one side of the fold.
std::vector<Point2> pageCurlRegion(double t, double pageWidth, double pageHeight, int samples = 40);

}  // namespace motion
