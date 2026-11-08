// easing.h - easing functions for the Digital Scrapbook animations (Checkpoint 2, Option 5).
//
// An easing function reshapes time. The animation loop gives every frame a linear time t in
// [0,1]; an easing turns it into a "progress" value p = ease(t), and the animation then uses p
// instead of t (e.g. position = lerp(start, end, p)). The shape of the curve controls how the
// motion FEELS: slow start, slow finish, bounce on landing, and so on.
//
// Agreed interface for the whole team: every easing is   double ease(double t)
// All functions below satisfy f(0) = 0 and f(1) = 1 (checked in tests/test_easing_paths.cpp).
// t is clamped to [0,1] so a slightly-out-of-range frame time can never blow up.
#pragma once

namespace easing {

double clamp01(double t);

double linear(double t);

// Quadratic: p = t^2 (in), 1-(1-t)^2 (out), or the two joined (in-out).
double easeInQuad(double t);      // starts slow, ends fast
double easeOutQuad(double t);     // starts fast, ends slow
double easeInOutQuad(double t);   // slow - fast - slow

// Cubic: a stronger version of the same shapes (p = t^3 ...).
double easeInCubic(double t);
double easeOutCubic(double t);
double easeInOutCubic(double t);

// Cubic Bezier easing - the same idea as CSS cubic-bezier(x1, y1, x2, y2). The curve runs from
// (0,0) to (1,1) with two control points (x1,y1) and (x2,y2); x is time, y is progress.
// x1 and x2 must be in [0,1]; y1 and y2 may leave [0,1] to give an overshoot.
// Example: cubicBezierEase(0.42, 0.0, 0.58, 1.0, t) is the classic "ease-in-out".
double cubicBezierEase(double x1, double y1, double x2, double y2, double t);

// Snap-and-settle for the sticker drop.
double easeOutBounce(double t);   // lands and bounces a few times, never goes past 1
double easeOutBack(double t);     // overshoots 1 by about 10% and settles back (peaks above 1)

}  // namespace easing
