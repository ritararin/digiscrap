#pragma once

#include "core/mat3.h"

// Speech Bubble:
Polygon makeSpeechBubble(
    Vec2 center,
    double width,
    double height
);

// Burst Badge:
Polygon makeBurstBadge(
    Vec2 center,
    double outerRadius,
    double innerRadius,
    int points = 12
);

// Ticket:
Polygon makeTicket(
    Vec2 center,
    double width,
    double height,
    double notchDepth = 16.0
);