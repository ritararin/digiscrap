// easing.cpp - see easing.h for the overview.
#include "easing.h"
#include <cmath>

namespace easing {

double clamp01(double t) { return t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t); }

double linear(double t) { return clamp01(t); }

double easeInQuad(double t) {
    t = clamp01(t);
    return t * t;
}

double easeOutQuad(double t) {
    t = clamp01(t);
    return 1.0 - (1.0 - t) * (1.0 - t);
}

double easeInOutQuad(double t) {
    t = clamp01(t);
    // First half: accelerate (2t^2). Second half: mirror it so we decelerate into 1.
    return t < 0.5 ? 2.0 * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) / 2.0;
}

double easeInCubic(double t) {
    t = clamp01(t);
    return t * t * t;
}

double easeOutCubic(double t) {
    t = clamp01(t);
    const double u = 1.0 - t;
    return 1.0 - u * u * u;
}

double easeInOutCubic(double t) {
    t = clamp01(t);
    return t < 0.5 ? 4.0 * t * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 3.0) / 2.0;
}

// ---------------------------------------------------------------------------------------------
// Cubic Bezier easing.
// The Bezier curve B(s), s in [0,1], has control points (0,0), (x1,y1), (x2,y2), (1,1):
//     x(s) = 3(1-s)^2 s x1 + 3(1-s) s^2 x2 + s^3
//     y(s) = 3(1-s)^2 s y1 + 3(1-s) s^2 y2 + s^3
// We are GIVEN the time t (an x value) and need the progress (the matching y). So we first find
// the curve parameter s with x(s) = t, then return y(s).
// x(s) is increasing when x1,x2 are in [0,1], so a root-finder works: Newton's method is fast,
// and bisection is the safe fallback if Newton's slope gets too flat.
// ---------------------------------------------------------------------------------------------
namespace {

double bez(double a, double b, double s) {  // one coordinate of the curve with end values 0 and 1
    const double u = 1.0 - s;
    return 3.0 * u * u * s * a + 3.0 * u * s * s * b + s * s * s;
}

double bezSlope(double a, double b, double s) {  // derivative d/ds
    const double u = 1.0 - s;
    return 3.0 * u * u * a + 6.0 * u * s * (b - a) + 3.0 * s * s * (1.0 - b);
}

}  // namespace

double cubicBezierEase(double x1, double y1, double x2, double y2, double t) {
    t = clamp01(t);
    if (t == 0.0) return 0.0;  // exact end values, so f(0)=0 and f(1)=1 hold with no rounding
    if (t == 1.0) return 1.0;

    // Newton's method on x(s) - t = 0, starting from s = t.
    double s = t;
    for (int i = 0; i < 8; ++i) {
        const double err = bez(x1, x2, s) - t;
        if (std::fabs(err) < 1e-12) return bez(y1, y2, s);
        const double slope = bezSlope(x1, x2, s);
        if (std::fabs(slope) < 1e-6) break;  // too flat - fall back to bisection
        s -= err / slope;
        if (s < 0.0 || s > 1.0) break;       // wandered out of range - fall back to bisection
    }

    // Bisection: halve the interval [lo,hi] around the root until it is tiny.
    double lo = 0.0, hi = 1.0;
    s = t;
    for (int i = 0; i < 60; ++i) {
        const double x = bez(x1, x2, s);
        if (std::fabs(x - t) < 1e-12) break;
        if (x < t) lo = s; else hi = s;
        s = 0.5 * (lo + hi);
    }
    return bez(y1, y2, s);
}

// ---------------------------------------------------------------------------------------------
// Bounce: the standard "ball dropped on the floor" curve, made of parabolas. Each bounce is a
// smaller parabola that touches 1.0 again. Values stay within [0,1].
// ---------------------------------------------------------------------------------------------
double easeOutBounce(double t) {
    t = clamp01(t);
    const double n1 = 7.5625, d1 = 2.75;
    if (t < 1.0 / d1) {
        return n1 * t * t;
    } else if (t < 2.0 / d1) {
        t -= 1.5 / d1;
        return n1 * t * t + 0.75;
    } else if (t < 2.5 / d1) {
        t -= 2.25 / d1;
        return n1 * t * t + 0.9375;
    } else {
        t -= 2.625 / d1;
        return n1 * t * t + 0.984375;
    }
}

// ---------------------------------------------------------------------------------------------
// Back (overshoot): swings past the target and settles back, like a sticker pressed down firmly.
// c1 controls how far it overshoots (1.70158 gives about 10%). This is a cubic in (t-1).
// ---------------------------------------------------------------------------------------------
double easeOutBack(double t) {
    t = clamp01(t);
    const double c1 = 1.70158, c3 = c1 + 1.0;
    const double u = t - 1.0;
    return 1.0 + c3 * u * u * u + c1 * u * u;
}

}  // namespace easing
