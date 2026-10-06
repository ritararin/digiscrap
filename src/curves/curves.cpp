#include "curves/curves.h"

#include <cmath>
#include <algorithm>

// CUBIC BEZIER:

// Formula:
// B(t) = (1-t)^3 P0
//      + 3(1-t)^2 t P1
//      + 3(1-t)t^2 P2
//      + t^3 P3
//
// t ranges from 0 to 1.

Polygon bezierCubic(
    Vec2 p0,
    Vec2 p1,
    Vec2 p2,
    Vec2 p3,
    int segments
) {
    Polygon curve;

    segments = std::max(1, segments);
    curve.reserve(segments + 1);

    for (int i = 0; i <= segments; i++) {
        double t = static_cast<double>(i) / segments;
        double u = 1.0 - t;

        double b0 = u * u * u;
        double b1 = 3.0 * u * u * t;
        double b2 = 3.0 * u * t * t;
        double b3 = t * t * t;

        Vec2 point;

        point.x =
            b0 * p0.x +
            b1 * p1.x +
            b2 * p2.x +
            b3 * p3.x;

        point.y =
            b0 * p0.y +
            b1 * p1.y +
            b2 * p2.y +
            b3 * p3.y;

        curve.push_back(point);
    }

    return curve;
}


// CATMULL-ROM SPLINE:
// Formula:
//
// P(t) = 0.5 [
//       2P1
//       + (-P0 + P2)t
//       + (2P0 - 5P1 + 4P2 - P3)t^2
//       + (-P0 + 3P1 - 3P2 + P3)t^3
// ]
//
// Each section uses four neighbouring control points.
// The curve passes through P1 and P2.

Polygon catmullRom(
    const std::vector<Vec2>& pts,
    int segmentsPerSpan
) {
    Polygon curve;

    if (pts.size() < 2)
        return pts;

    segmentsPerSpan = std::max(1, segmentsPerSpan);

    // Duplicate the first and last points.
    // This allows the spline to start at pts[0]
    // and finish at pts.back().
    std::vector<Vec2> control;

    control.push_back(pts.front());

    for (const Vec2& p : pts)
        control.push_back(p);

    control.push_back(pts.back());

    for (size_t i = 0; i + 3 < control.size(); i++) {
        Vec2 p0 = control[i];
        Vec2 p1 = control[i + 1];
        Vec2 p2 = control[i + 2];
        Vec2 p3 = control[i + 3];

        for (int j = 0; j < segmentsPerSpan; j++) {
            double t =
                static_cast<double>(j) / segmentsPerSpan;

            double t2 = t * t;
            double t3 = t2 * t;

            Vec2 point;

            point.x = 0.5 * (
                2.0 * p1.x
                + (-p0.x + p2.x) * t
                + (2.0 * p0.x
                   - 5.0 * p1.x
                   + 4.0 * p2.x
                   - p3.x) * t2
                + (-p0.x
                   + 3.0 * p1.x
                   - 3.0 * p2.x
                   + p3.x) * t3
            );

            point.y = 0.5 * (
                2.0 * p1.y
                + (-p0.y + p2.y) * t
                + (2.0 * p0.y
                   - 5.0 * p1.y
                   + 4.0 * p2.y
                   - p3.y) * t2
                + (-p0.y
                   + 3.0 * p1.y
                   - 3.0 * p2.y
                   + p3.y) * t3
            );

            curve.push_back(point);
        }
    }

    // Ensure exact final point is included.
    curve.push_back(pts.back());

    return curve;
}


// HEART USING TWO CUBIC BEZIER CURVES:

Polygon makeBezierHeart(
    Vec2 center,
    double size,
    int segments
) {
    Polygon heart;

    double x = center.x;
    double y = center.y;

    double w = size;
    double h = size;

    Vec2 top    {x, y - h * 0.20};
    Vec2 bottom {x, y + h * 0.55};

    // Left half
    Polygon left = bezierCubic(
        top,
        {x - w * 0.65, y - h * 0.75},
        {x - w * 0.85, y + h * 0.15},
        bottom,
        segments
    );

    // Right half
    Polygon right = bezierCubic(
        bottom,
        {x + w * 0.85, y + h * 0.15},
        {x + w * 0.65, y - h * 0.75},
        top,
        segments
    );

    heart.insert(
        heart.end(),
        left.begin(),
        left.end()
    );

    // Skip duplicated bottom point.
    heart.insert(
        heart.end(),
        right.begin() + 1,
        right.end()
    );

    return heart;
}

// CLOUD USING CATMULL-ROM:

Polygon makeCloud(
    Vec2 center,
    double width,
    double height,
    int segmentsPerSpan
) {
    double x = center.x;
    double y = center.y;

    std::vector<Vec2> controls = {
        {x - width * 0.50, y + height * 0.15},
        {x - width * 0.42, y - height * 0.05},
        {x - width * 0.28, y - height * 0.18},
        {x - width * 0.12, y - height * 0.38},
        {x + width * 0.08, y - height * 0.35},
        {x + width * 0.22, y - height * 0.22},
        {x + width * 0.40, y - height * 0.15},
        {x + width * 0.50, y + height * 0.08},
        {x + width * 0.40, y + height * 0.28},
        {x + width * 0.10, y + height * 0.32},
        {x - width * 0.20, y + height * 0.30},
        {x - width * 0.50, y + height * 0.15}
    };

    return catmullRom(
        controls,
        segmentsPerSpan
    );
}


// CURVY STAR USING CATMULL-ROM:

Polygon makeCurvyStar(
    Vec2 center,
    double outerRadius,
    double innerRadius,
    int points,
    int segmentsPerSpan
) {
    if (points < 3)
        points = 3;

    constexpr double PI =
        3.14159265358979323846;

    std::vector<Vec2> controls;

    int total = points * 2;

    for (int i = 0; i < total; i++) {
        double angle =
            -PI / 2.0 +
            i * PI / points;

        double radius =
            (i % 2 == 0)
            ? outerRadius
            : innerRadius;

        controls.push_back({
            center.x + std::cos(angle) * radius,
            center.y + std::sin(angle) * radius
        });
    }

    // Catmull-Rom needs neighbouring points to generate
    // a smooth CLOSED curve.
    std::vector<Vec2> closed;

    closed.push_back(
        controls[controls.size() - 2]
    );

    closed.push_back(
        controls.back()
    );

    for (const Vec2& p : controls)
        closed.push_back(p);

    closed.push_back(
        controls[0]
    );

    closed.push_back(
        controls[1]
    );

    Polygon spline =
        catmullRom(
            closed,
            segmentsPerSpan
        );

    /*
     * Remove the extra lead-in and lead-out areas introduced
     * only to provide Catmull-Rom with neighbouring points.
     */

    size_t skip =
        static_cast<size_t>(
            2 * segmentsPerSpan
        );

    if (spline.size() > 2 * skip) {
        return Polygon(
            spline.begin() + skip,
            spline.end() - skip
        );
    }

    return spline;
}