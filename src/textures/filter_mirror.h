#pragma once
#include "core/framebuffer.h"

// CPU mirror of the fragment-shader filter maths (float, 0..1 range).
// The C++ filters in colour/colour.cpp stay the reference; the test program
// checks this mirror against them. The GLSL in texture_renderer.cpp uses
// exactly the same formulas.
enum FilterKind { FILTER_NONE = 0, FILTER_GRAYSCALE, FILTER_SEPIA, FILTER_TINT,
                  FILTER_BRIGHTNESS, FILTER_CONTRAST, FILTER_INVERT, FILTER_COUNT };

struct FilterParams {
    FilterKind kind = FILTER_NONE;
    float strength  = 1.0f;                 // 0 = original, 1 = fully filtered
    Color tint      = {255, 140, 180, 255}; // FILTER_TINT colour
    float amount    = 0.2f;                 // brightness delta (0..1 of full range) or contrast factor
};

Color applyFilterMirror(Color c, const FilterParams& f);
const char* filterName(FilterKind k);
