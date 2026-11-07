#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "core/framebuffer.h"   // Color
#include "core/mat3.h"          // Vec2

// Clamp t into [0, 1].
inline double clamp01(double t) { return t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t); }

// Linear interpolation: returns a at t = 0 and exactly b at t = 1.
inline double lerp(double a, double b, double t) { return a + (b - a) * t; }

inline Vec2 lerp(Vec2 a, Vec2 b, double t) {
    return { lerp(a.x, b.x, t), lerp(a.y, b.y, t) };
}

// Per-channel colour lerp (alpha included). Rounds to the nearest value.
inline Color lerp(Color a, Color b, double t) {
    auto ch = [t](uint8_t x, uint8_t y) {
        double v = lerp((double)x, (double)y, t);
        return (uint8_t)std::lround(std::clamp(v, 0.0, 255.0));
    };
    return { ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b), ch(a.a, b.a) };
}