#include "textures/image_loader.h"
#include <fstream>
#include <vector>
#include <cmath>
#include <cctype>

static uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }

bool loadBMP(const std::string& path, Framebuffer& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (d.size() < 54 || d[0] != 'B' || d[1] != 'M') return false;

    uint32_t dataOffset = rd32(&d[10]);
    int32_t  w = (int32_t)rd32(&d[18]);
    int32_t  h = (int32_t)rd32(&d[22]);
    uint16_t bpp = rd16(&d[28]);
    uint32_t compression = rd32(&d[30]);
    if (compression != 0 || (bpp != 24 && bpp != 32)) return false;

    bool topDown = h < 0;                 // negative height = rows stored top first
    if (topDown) h = -h;
    if (w <= 0 || h <= 0 || w > 16384 || h > 16384) return false;

    int bytes = bpp / 8;
    size_t rowSize = ((size_t)w * bytes + 3) & ~(size_t)3;   // rows padded to 4 bytes
    if (dataOffset + rowSize * h > d.size()) return false;

    Framebuffer img(w, h, {0, 0, 0, 255});
    for (int y = 0; y < h; ++y) {
        int srcRow = topDown ? y : (h - 1 - y);              // BMP is usually bottom-up
        const uint8_t* row = &d[dataOffset + rowSize * srcRow];
        for (int x = 0; x < w; ++x) {
            const uint8_t* p = row + x * bytes;              // stored as B, G, R(, A)
            img.pixels[y * w + x] = { p[2], p[1], p[0], 255 };
        }
    }
    out = img;
    return true;
}

// Reads the next integer in a PPM header, skipping spaces and # comments.
static bool ppmInt(const std::vector<uint8_t>& d, size_t& i, int& v) {
    while (i < d.size()) {
        if (d[i] == '#') { while (i < d.size() && d[i] != '\n') ++i; }
        else if (std::isspace(d[i])) ++i;
        else break;
    }
    if (i >= d.size() || !std::isdigit(d[i])) return false;
    v = 0;
    while (i < d.size() && std::isdigit(d[i])) { v = v * 10 + (d[i] - '0'); ++i; }
    return true;
}

bool loadPPM(const std::string& path, Framebuffer& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (d.size() < 7 || d[0] != 'P' || d[1] != '6') return false;
    size_t i = 2;
    int w, h, maxv;
    if (!ppmInt(d, i, w) || !ppmInt(d, i, h) || !ppmInt(d, i, maxv)) return false;
    if (maxv != 255 || w <= 0 || h <= 0 || w > 16384 || h > 16384) return false;
    ++i;                                                     // single whitespace after maxval
    if (i + (size_t)w * h * 3 > d.size()) return false;
    Framebuffer img(w, h, {0, 0, 0, 255});
    for (int k = 0; k < w * h; ++k)
        img.pixels[k] = { d[i + 3 * k], d[i + 3 * k + 1], d[i + 3 * k + 2], 255 };
    out = img;
    return true;
}

bool loadImage(const std::string& path, Framebuffer& out) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = path.substr(dot + 1);
    for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
    if (ext == "bmp") return loadBMP(path, out);
    if (ext == "ppm") return loadPPM(path, out);
    return false;
}

Framebuffer resizeBilinear(const Framebuffer& src, int w, int h) {
    Framebuffer dst(w, h, {0, 0, 0, 255});
    for (int y = 0; y < h; ++y) {
        double sy = (y + 0.5) * src.height / h - 0.5;
        int y0 = (int)std::floor(sy); double fy = sy - y0;
        int ya = std::max(0, std::min(src.height - 1, y0)), yb = std::max(0, std::min(src.height - 1, y0 + 1));
        for (int x = 0; x < w; ++x) {
            double sx = (x + 0.5) * src.width / w - 0.5;
            int x0 = (int)std::floor(sx); double fx = sx - x0;
            int xa = std::max(0, std::min(src.width - 1, x0)), xb = std::max(0, std::min(src.width - 1, x0 + 1));
            const Color& c00 = src.pixels[ya * src.width + xa];
            const Color& c10 = src.pixels[ya * src.width + xb];
            const Color& c01 = src.pixels[yb * src.width + xa];
            const Color& c11 = src.pixels[yb * src.width + xb];
            auto mix = [&](uint8_t a, uint8_t b, uint8_t c, uint8_t e) {
                double top = a + (b - a) * fx, bot = c + (e - c) * fx;
                return (uint8_t)std::lround(top + (bot - top) * fy);
            };
            dst.pixels[y * w + x] = { mix(c00.r, c10.r, c01.r, c11.r),
                                      mix(c00.g, c10.g, c01.g, c11.g),
                                      mix(c00.b, c10.b, c01.b, c11.b), 255 };
        }
    }
    return dst;
}

Framebuffer makeDemoPhoto(int w, int h) {
    Framebuffer img(w, h, {0, 0, 0, 255});
    for (int y = 0; y < h; ++y) {
        double t = (double)y / (h - 1);
        for (int x = 0; x < w; ++x) {
            Color c = { (uint8_t)(110 + 120 * t), (uint8_t)(170 + 50 * t), (uint8_t)(235 - 30 * t), 255 }; // sky
            double dx = x - 0.72 * w, dy = y - 0.28 * h;
            if (dx * dx + dy * dy < 0.011 * w * w) c = {255, 226, 120, 255};                               // sun
            double hill = 0.70 * h + 0.07 * h * std::sin(x * 6.2831 / w * 1.5);
            if (y > hill) c = { (uint8_t)(70 + 30 * (1 - t)), (uint8_t)(150 - 30 * t), 85, 255 };           // hills
            img.pixels[y * w + x] = c;
        }
    }
    return img;
}
