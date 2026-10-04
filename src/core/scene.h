#pragma once
#include <functional>
#include <string>
#include <vector>
#include "core/framebuffer.h"

class Scene {
    struct Layer { std::string name; std::function<void(Framebuffer&)> draw; };
    std::vector<Layer> layers;
public:
    void add(std::string name, std::function<void(Framebuffer&)> fn) {
        layers.push_back({std::move(name), std::move(fn)});
    }
    void render(Framebuffer& fb) const { for (auto& l : layers) l.draw(fb); }  // first = bottom
};