#pragma once
#include <functional>
#include "core/framebuffer.h"
#include "animation/lerp.h"

// The agreed shape of every animation: draw the page at time t (0..1).
using RenderFrameFn = std::function<void(Framebuffer&, double)>;

struct Timeline {
    int    frames = 24;     // total number of frames (>= 1)
    double fps    = 12.0;

    // Time for frame i (0-based): exactly 0 on the first frame and
    // exactly 1 on the last. A one-frame timeline shows the end state (t = 1).
    double tAt(int i) const {
        if (frames <= 1) return 1.0;
        return (double)i / (double)(frames - 1);
    }

    double durationSeconds() const { return frames / fps; }

    void forEach(const std::function<void(int index, double t)>& fn) const {
        for (int i = 0; i < frames; i++) fn(i, tAt(i));
    }
};

// Map global t onto a sub-range: 0 before `start`, 1 after `end`, linear between.
// Lets one sequence run several steps ("heart slides in between t=0.25 and 1").
inline double segmentT(double t, double start, double end) {
    if (end <= start) return t >= end ? 1.0 : 0.0;
    return clamp01((t - start) / (end - start));
}