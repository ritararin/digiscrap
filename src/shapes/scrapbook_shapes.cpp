#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include "shapes/scrapbook_shapes.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// Speech Bubble:
Polygon makeSpeechBubble(
    Vec2 center,
    double width,
    double height
) {
    double left   = center.x - width / 2.0;
    double right  = center.x + width / 2.0;
    double top    = center.y - height / 2.0;
    double bottom = center.y + height / 2.0;

    double tailWidth = width * 0.18;
    double tailDepth = height * 0.35;

    Polygon p = {
        {left, top},
        {right, top},
        {right, bottom},
        {center.x + tailWidth, bottom},
        {center.x, bottom + tailDepth},
        {center.x - tailWidth, bottom},
        {left, bottom}
    };

    return p;
}


// Burst Badge:
Polygon makeBurstBadge(
    Vec2 center,
    double outerRadius,
    double innerRadius,
    int points
) {
    points = std::max(points, 3);

    Polygon p;
    int totalVertices = points * 2;

    for (int i = 0; i < totalVertices; i++) {
        double angle =
            -M_PI / 2.0
            + i * M_PI / points;

        double radius =
            (i % 2 == 0)
            ? outerRadius
            : innerRadius;

        p.push_back({
            center.x + std::cos(angle) * radius,
            center.y + std::sin(angle) * radius
        });
    }

    return p;
}


// Ticket:
Polygon makeTicket(
    Vec2 center,
    double width,
    double height,
    double notchDepth
) {
    double left   = center.x - width / 2.0;
    double right  = center.x + width / 2.0;
    double top    = center.y - height / 2.0;
    double bottom = center.y + height / 2.0;

    double midY = center.y;

    Polygon p = {
        {left, top},
        {right, top},

        {right, midY - notchDepth},
        {right - notchDepth, midY},
        {right, midY + notchDepth},

        {right, bottom},
        {left, bottom},

        {left, midY + notchDepth},
        {left + notchDepth, midY},
        {left, midY - notchDepth}
    };

    return p;
}