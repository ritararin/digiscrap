#include "textures/filter_mirror.h"
#include <algorithm>
#include <cmath>

static float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

Color applyFilterMirror(Color c, const FilterParams& f) {
    float r = c.r / 255.0f, g = c.g / 255.0f, b = c.b / 255.0f;
    float fr = r, fg = g, fb = b;
    switch (f.kind) {
    case FILTER_GRAYSCALE: { float y = 0.299f * r + 0.587f * g + 0.114f * b; fr = fg = fb = y; break; }
    case FILTER_SEPIA:
        fr = 0.393f * r + 0.769f * g + 0.189f * b;
        fg = 0.349f * r + 0.686f * g + 0.168f * b;
        fb = 0.272f * r + 0.534f * g + 0.131f * b; break;
    case FILTER_TINT:  // full-strength result is the tint colour; "strength" blends toward it
        fr = f.tint.r / 255.0f; fg = f.tint.g / 255.0f; fb = f.tint.b / 255.0f; break;
    case FILTER_BRIGHTNESS: fr = r + f.amount; fg = g + f.amount; fb = b + f.amount; break;
    case FILTER_CONTRAST:   fr = 0.5f + (r - 0.5f) * f.amount; fg = 0.5f + (g - 0.5f) * f.amount; fb = 0.5f + (b - 0.5f) * f.amount; break;
    case FILTER_INVERT:     fr = 1 - r; fg = 1 - g; fb = 1 - b; break;
    default: break;
    }
    float s = clamp01(f.strength);
    auto out = [&](float orig, float filt) { return (uint8_t)std::lround(clamp01(orig + (filt - orig) * s) * 255.0f); };
    return { out(r, fr), out(g, fg), out(b, fb), c.a };
}

const char* filterName(FilterKind k) {
    static const char* n[] = {"none", "grayscale", "sepia", "tint", "brightness", "contrast", "invert"};
    return (k >= 0 && k < FILTER_COUNT) ? n[k] : "?";
}
