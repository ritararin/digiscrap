#pragma once

#include "core/framebuffer.h"

// Smoothly interpolate from one colour to another.
// t = 0 returns 'from', t = 1 returns 'to'.
Color animateColour(
    Color from,
    Color to,
    double t
);

// Crossfade between two complete framebuffers.
// t = 0 shows only 'from'.
// t = 1 shows only 'to'.
void crossfade(
    Framebuffer& output,
    const Framebuffer& from,
    const Framebuffer& to,
    double t
);

// Fade an image towards a chosen colour.
// Example: fade to black, white, cream, etc.
void fadeToColour(
    Framebuffer& output,
    const Framebuffer& source,
    Color fadeColour,
    double t
);

// Fade an image in from a solid colour.
void fadeFromColour(
    Framebuffer& output,
    const Framebuffer& source,
    Color fadeColour,
    double t
);