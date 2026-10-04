#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include <algorithm>

struct Color { uint8_t r, g, b, a = 255; };

class Framebuffer {
public:
    int width, height;
    std::vector<Color> pixels;

    Framebuffer(int w, int h, Color bg = {255,255,255})
        : width(w), height(h), pixels(w * h, bg) {}

    void clear(Color c) { std::fill(pixels.begin(), pixels.end(), c); }

    void setPixel(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= width || y >= height) return;
        pixels[y * width + x] = c;
    }

    bool savePPM(const std::string& path) const {
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        f << "P6\n" << width << " " << height << "\n255\n";
        for (const auto& p : pixels) { f.put(p.r); f.put(p.g); f.put(p.b); }
        return true;
    }

    bool saveBMP(const std::string& path) const {   // opens everywhere, incl. Preview
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        int rowSize = (width * 3 + 3) & ~3;
        uint32_t dataSize = rowSize * height;
        uint8_t h[54] = {0};
        h[0] = 'B'; h[1] = 'M';
        auto w32 = [&](int o, uint32_t v){ for (int i = 0; i < 4; i++) h[o+i] = (v >> (8*i)) & 0xFF; };
        w32(2, 54 + dataSize); w32(10, 54); w32(14, 40);
        w32(18, width); w32(22, height); h[26] = 1; h[28] = 24; w32(34, dataSize);
        f.write((char*)h, 54);
        std::vector<uint8_t> row(rowSize, 0);
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; x++) {
                Color c = pixels[y * width + x];
                row[x*3] = c.b; row[x*3+1] = c.g; row[x*3+2] = c.r;
            }
            f.write((char*)row.data(), rowSize);
        }
        return true;
    }
};