#include "transitions/transitions.h"

#include <algorithm>
#include <cmath>

// Helper functions:

static double clamp01(double t) {
    return std::clamp(t, 0.0, 1.0);
}


static uint8_t interpolateChannel(
    uint8_t from,
    uint8_t to,
    double t
) {
    t = clamp01(t);

    double value =
        static_cast<double>(from)
        +
        (
            static_cast<double>(to)
            -
            static_cast<double>(from)
        ) * t;

    return static_cast<uint8_t>(
        std::lround(
            std::clamp(
                value,
                0.0,
                255.0
            )
        )
    );
}

// COLOUR ANIMATION:

Color animateColour(
    Color from,
    Color to,
    double t
) {
    t = clamp01(t);

    return {
        interpolateChannel(from.r, to.r, t),
        interpolateChannel(from.g, to.g, t),
        interpolateChannel(from.b, to.b, t),
        interpolateChannel(from.a, to.a, t)
    };
}

// CROSSFADE:

void crossfade(
    Framebuffer& output,
    const Framebuffer& from,
    const Framebuffer& to,
    double t
) {
    t = clamp01(t);

    // Both source framebuffers must match the output dimensions.
    if (
        output.width != from.width ||
        output.height != from.height ||
        output.width != to.width ||
        output.height != to.height
    ) {
        return;
    }

    for (size_t i = 0; i < output.pixels.size(); i++) {

        output.pixels[i] =
            animateColour(
                from.pixels[i],
                to.pixels[i],
                t
            );
    }
}

// FADE TO COLOUR:

void fadeToColour(
    Framebuffer& output,
    const Framebuffer& source,
    Color fadeColour,
    double t
) {
    t = clamp01(t);

    if (
        output.width != source.width ||
        output.height != source.height
    ) {
        return;
    }

    for (size_t i = 0; i < output.pixels.size(); i++) {

        output.pixels[i] =
            animateColour(
                source.pixels[i],
                fadeColour,
                t
            );
    }
}


// FADE FROM COLOUR:

void fadeFromColour(
    Framebuffer& output,
    const Framebuffer& source,
    Color fadeColour,
    double t
) {
    t = clamp01(t);

    if (
        output.width != source.width ||
        output.height != source.height
    ) {
        return;
    }

    for (size_t i = 0; i < output.pixels.size(); i++) {

        output.pixels[i] =
            animateColour(
                fadeColour,
                source.pixels[i],
                t
            );
    }
}