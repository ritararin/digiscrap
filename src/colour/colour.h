#pragma once
#include <functional>
#include "core/framebuffer.h"

// Filters operate on straight (not premultiplied) RGB and preserve input alpha.
Color grayscale(Color colour);
Color sepia(Color colour);
// Strength is clamped to [0, 1]; the tint colour's alpha is not used.
Color tint(Color colour, Color tintColour, double strength);
Color brightness(Color colour, int delta);
// A negative factor is treated as zero; factor 1 leaves RGB unchanged.
Color contrast(Color colour, double factor);
Color invert(Color colour);

// Source-over for straight RGBA. With opaque destination this reduces to
// src * (src.a/255) + dst * (1-src.a/255) and produces opaque output.
Color alphaBlend(Color source, Color destination);

using ColourFilter = std::function<Color(Color)>;
void applyFilter(Framebuffer& image, const ColourFilter& filter);
// Half-open pixel region [x0, x1) x [y0, y1), clamped to image bounds.
// Empty/reversed regions and empty callbacks do nothing.
void applyFilter(Framebuffer& image, int x0, int y0, int x1, int y1,
                 const ColourFilter& filter);