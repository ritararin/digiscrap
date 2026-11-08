// paths.cpp - see paths.h for the overview.
#include "paths.h"
#include <algorithm>
#include <cmath>

namespace motion {

namespace {
const double kPi = 3.14159265358979323846;  // not M_PI: that needs a flag on MSVC
double clamp01(double t) { return t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t); }
}  // namespace

// ---- Bezier ---------------------------------------------------------------------------------
// Bernstein form:  B(t) = sum_i C(n,i) (1-t)^(n-i) t^i P_i
Point2 quadBezier(Point2 p0, Point2 p1, Point2 p2, double t) {
    t = clamp01(t);
    const double u = 1.0 - t;
    const double a = u * u, b = 2.0 * u * t, c = t * t;
    return {a * p0.x + b * p1.x + c * p2.x, a * p0.y + b * p1.y + c * p2.y};
}

Point2 cubicBezier(Point2 p0, Point2 p1, Point2 p2, Point2 p3, double t) {
    t = clamp01(t);
    const double u = 1.0 - t;
    const double a = u * u * u, b = 3.0 * u * u * t, c = 3.0 * u * t * t, d = t * t * t;
    return {a * p0.x + b * p1.x + c * p2.x + d * p3.x, a * p0.y + b * p1.y + c * p2.y + d * p3.y};
}

Point2 cubicBezierTangent(Point2 p0, Point2 p1, Point2 p2, Point2 p3, double t) {
    t = clamp01(t);
    const double u = 1.0 - t;
    // B'(t) = 3(1-t)^2 (P1-P0) + 6(1-t)t (P2-P1) + 3t^2 (P3-P2)
    const double a = 3.0 * u * u, b = 6.0 * u * t, c = 3.0 * t * t;
    return {a * (p1.x - p0.x) + b * (p2.x - p1.x) + c * (p3.x - p2.x),
            a * (p1.y - p0.y) + b * (p2.y - p1.y) + c * (p3.y - p2.y)};
}

// ---- Catmull-Rom ----------------------------------------------------------------------------
// Between waypoints P1 and P2, with neighbours P0 and P3, the curve is
//     0.5 * ( 2P1 + (-P0+P2) t + (2P0-5P1+4P2-P3) t^2 + (-P0+3P1-3P2+P3) t^3 ),  t in [0,1]
// At t=0 it equals P1 and at t=1 it equals P2, so it passes through every waypoint. The tangent
// at P1 is (P2-P0)/2, which is why the curve flows smoothly from one segment into the next.
// At the two ends there is no neighbour, so we mirror one (P_-1 = 2P0 - P1).
CatmullRomPath::CatmullRomPath(std::vector<Point2> waypoints) : pts_(std::move(waypoints)) {
    if (pts_.size() < 2) pts_.resize(2, pts_.empty() ? Point2{} : pts_[0]);
}

Point2 CatmullRomPath::at(double t) const {
    t = clamp01(t);
    const int segments = static_cast<int>(pts_.size()) - 1;
    double scaled = t * segments;
    int i = static_cast<int>(std::floor(scaled));
    if (i >= segments) i = segments - 1;  // t == 1 belongs to the last segment's end
    const double local = scaled - i;

    const Point2 p1 = pts_[i], p2 = pts_[i + 1];
    const Point2 p0 = (i == 0) ? Point2{2 * p1.x - p2.x, 2 * p1.y - p2.y} : pts_[i - 1];
    const Point2 p3 = (i + 2 >= static_cast<int>(pts_.size()))
                          ? Point2{2 * p2.x - p1.x, 2 * p2.y - p1.y}
                          : pts_[i + 2];

    const double t2 = local * local, t3 = t2 * local;
    auto coord = [&](double a0, double a1, double a2, double a3) {
        return 0.5 * (2.0 * a1 + (-a0 + a2) * local + (2.0 * a0 - 5.0 * a1 + 4.0 * a2 - a3) * t2 +
                      (-a0 + 3.0 * a1 - 3.0 * a2 + a3) * t3);
    };
    return {coord(p0.x, p1.x, p2.x, p3.x), coord(p0.y, p1.y, p2.y, p3.y)};
}

// ---- Arc length -----------------------------------------------------------------------------
// Approximate the curve by a polyline of N straight pieces and add up their lengths. To find the
// t at distance d, find which piece d falls in and interpolate linearly inside it.
ArcLengthTable::ArcLengthTable(std::function<Point2(double)> curve, int samples) {
    samples = std::max(samples, 2);
    lengths_.resize(samples + 1, 0.0);
    Point2 prev = curve(0.0);
    for (int i = 1; i <= samples; ++i) {
        const Point2 cur = curve(static_cast<double>(i) / samples);
        lengths_[i] = lengths_[i - 1] + std::hypot(cur.x - prev.x, cur.y - prev.y);
        prev = cur;
    }
}

double ArcLengthTable::tAtFraction(double u) const {
    u = clamp01(u);
    const double target = u * lengths_.back();
    // First sample whose cumulative length reaches the target.
    auto it = std::lower_bound(lengths_.begin(), lengths_.end(), target);
    const size_t hi = static_cast<size_t>(it - lengths_.begin());
    if (hi == 0) return 0.0;
    const size_t lo = hi - 1;
    const double span = lengths_[hi] - lengths_[lo];
    const double frac = span > 0.0 ? (target - lengths_[lo]) / span : 0.0;
    return (static_cast<double>(lo) + frac) / static_cast<double>(lengths_.size() - 1);
}

// ---- Page curl ------------------------------------------------------------------------------
// The fold line is a cubic Bezier from the top edge to the bottom edge of the page.
//   x position of the fold:  fx = W * (1 - t)       (sweeps from the right edge to the left)
//   the two inner control points are pulled LEFT by "bow", so the fold bulges toward the side
//   the page is turning toward, like a page lifting at its middle.
// bow = 0 at t=0 and t=1 (the page is flat at the start and end) and largest mid-turn.
std::vector<Point2> pageCurlEdge(double t, double W, double H, int samples) {
    t = clamp01(t);
    samples = std::max(samples, 2);
    const double fx = W * (1.0 - t);
    const double bow = 0.18 * W * std::sin(kPi * t);  // 0 -> max -> 0

    const Point2 p0{fx, 0.0};
    const Point2 p1{fx - bow, H * 0.33};
    const Point2 p2{fx - bow, H * 0.66};
    const Point2 p3{fx, H};

    std::vector<Point2> edge;
    edge.reserve(samples);
    for (int i = 0; i < samples; ++i)
        edge.push_back(cubicBezier(p0, p1, p2, p3, static_cast<double>(i) / (samples - 1)));
    return edge;
}

std::vector<Point2> pageCurlRegion(double t, double W, double H, int samples) {
    (void)W;
    std::vector<Point2> poly = pageCurlEdge(t, W, H, samples);
    poly.push_back({0.0, H});  // bottom-left corner
    poly.push_back({0.0, 0.0});  // top-left corner
    return poly;
}

}  // namespace motion
