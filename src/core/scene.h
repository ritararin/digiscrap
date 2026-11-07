#pragma once
#include <functional>
#include <string>
#include <vector>
#include "core/framebuffer.h"
#include "animation/timeline.h"

// Ordered layers, drawn bottom to top (painter's algorithm).
// Layers can now be static, animated, or visible only for part of the timeline.
class Scene {
    struct Layer {
        std::string name;
        double start, end;                                 // visible while start <= t <= end
        std::function<void(Framebuffer&, double)> draw;    // receives the layer's local t
    };
    std::vector<Layer> layers;

public:
    // Static layer (CP1 behaviour): always drawn, ignores time.
    void add(std::string name, std::function<void(Framebuffer&)> fn) {
        layers.push_back({std::move(name), 0.0, 1.0,
                          [fn](Framebuffer& f, double) { fn(f); }});
    }

    // Animated layer: drawn on every frame and given the global t (0..1).
    void addAnimated(std::string name, std::function<void(Framebuffer&, double)> fn) {
        layers.push_back({std::move(name), 0.0, 1.0, std::move(fn)});
    }

    // Timed layer: only drawn while start <= t <= end.
    // It receives local t: 0 at `start`, 1 at `end`.
    void addTimed(std::string name, double start, double end,
                  std::function<void(Framebuffer&, double)> fn) {
        layers.push_back({std::move(name), start, end, std::move(fn)});
    }

    // t defaults to 1 so the old `scene.render(fb)` call still shows the finished page.
    void render(Framebuffer& fb, double t = 1.0) const {
        for (const auto& l : layers) {
            if (t < l.start || t > l.end) continue;
            l.draw(fb, segmentT(t, l.start, l.end));
        }
    }

    size_t size() const { return layers.size(); }
};