#include "colour/colour.h"
#include <algorithm>
#include <cmath>

namespace {
uint8_t channel(double value) {
    return static_cast<uint8_t>(std::lround(std::clamp(value, 0.0, 255.0)));
}
}

Color grayscale(Color colour) {
    uint8_t gray = channel(0.299 * colour.r + 0.587 * colour.g + 0.114 * colour.b);
    return {gray, gray, gray, colour.a};
}

Color sepia(Color colour) {
    // All three rows use the original RGB, not a partly converted colour.
    return {channel(0.393 * colour.r + 0.769 * colour.g + 0.189 * colour.b),
            channel(0.349 * colour.r + 0.686 * colour.g + 0.168 * colour.b),
            channel(0.272 * colour.r + 0.534 * colour.g + 0.131 * colour.b), colour.a};
}

Color tint(Color colour, Color tintColour, double strength) {
    strength = std::clamp(strength, 0.0, 1.0);
    return {channel(colour.r * (1.0 - strength) + tintColour.r * strength),
            channel(colour.g * (1.0 - strength) + tintColour.g * strength),
            channel(colour.b * (1.0 - strength) + tintColour.b * strength), colour.a};
}

Color brightness(Color colour, int delta) {
    return {channel(colour.r + static_cast<double>(delta)),
            channel(colour.g + static_cast<double>(delta)),
            channel(colour.b + static_cast<double>(delta)), colour.a};
}

Color contrast(Color colour, double factor) {
    factor = std::max(0.0, factor);
    return {channel(128.0 + (colour.r - 128.0) * factor),
            channel(128.0 + (colour.g - 128.0) * factor),
            channel(128.0 + (colour.b - 128.0) * factor), colour.a};
}

Color invert(Color colour) {
    return {static_cast<uint8_t>(255 - colour.r), static_cast<uint8_t>(255 - colour.g),
            static_cast<uint8_t>(255 - colour.b), colour.a};
}

Color alphaBlend(Color source, Color destination) {
    if (source.a == 0) return destination;
    if (source.a == 255) return source;
    double sourceAlpha = source.a / 255.0;
    double destinationWeight = (destination.a / 255.0) * (1.0 - sourceAlpha);
    double outputAlpha = sourceAlpha + destinationWeight;
    // Blend premultiplied contributions, then divide by output alpha to store
    // straight RGB again. source.a > 0 guarantees outputAlpha > 0.
    return {channel((source.r * sourceAlpha + destination.r * destinationWeight) / outputAlpha),
            channel((source.g * sourceAlpha + destination.g * destinationWeight) / outputAlpha),
            channel((source.b * sourceAlpha + destination.b * destinationWeight) / outputAlpha),
            channel(outputAlpha * 255.0)};
}

void applyFilter(Framebuffer& image, const ColourFilter& filter) {
    applyFilter(image, 0, 0, image.width, image.height, filter);
}

void applyFilter(Framebuffer& image, int x0, int y0, int x1, int y1,
                 const ColourFilter& filter) {
    if (!filter) return;
    x0 = std::max(0, x0); y0 = std::max(0, y0);
    x1 = std::min(image.width, x1); y1 = std::min(image.height, y1);
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            Color& pixel = image.pixels[y * image.width + x];
            pixel = filter(pixel);
        }
}