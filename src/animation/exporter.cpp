#include "animation/exporter.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

std::string frameFileName(const std::string& dir, const std::string& name, int index) {
    char num[16];
    std::snprintf(num, sizeof num, "%04d", index);
    return dir + "/" + name + "_" + num + ".bmp";
}

bool ensureDirectory(const std::string& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return !ec;
}

Framebuffer downscale(const Framebuffer& src, int newW, int newH) {
    Framebuffer out(newW, newH);
    for (int y = 0; y < newH; y++) {
        int y0 = y * src.height / newH;
        int y1 = std::max(y0 + 1, (y + 1) * src.height / newH);
        for (int x = 0; x < newW; x++) {
            int x0 = x * src.width / newW;
            int x1 = std::max(x0 + 1, (x + 1) * src.width / newW);
            long r = 0, g = 0, b = 0, n = 0;
            for (int sy = y0; sy < y1 && sy < src.height; sy++)
                for (int sx = x0; sx < x1 && sx < src.width; sx++) {
                    const Color& c = src.pixels[sy * src.width + sx];
                    r += c.r; g += c.g; b += c.b; n++;
                }
            if (n) out.setPixel(x, y, {(uint8_t)(r / n), (uint8_t)(g / n), (uint8_t)(b / n)});
        }
    }
    return out;
}

Framebuffer makeContactSheet(const std::vector<Framebuffer>& thumbs,
                             int columns, int gap, Color bg) {
    if (thumbs.empty()) return Framebuffer(1, 1, bg);
    if (columns < 1) columns = 1;
    int cw = 0, ch = 0;
    for (const auto& t : thumbs) { cw = std::max(cw, t.width); ch = std::max(ch, t.height); }
    int n = (int)thumbs.size();
    int cols = std::min(columns, n);
    int rows = (n + cols - 1) / cols;
    Framebuffer sheet(cols * cw + (cols + 1) * gap, rows * ch + (rows + 1) * gap, bg);
    for (int i = 0; i < n; i++) {
        int ox = gap + (i % cols) * (cw + gap);
        int oy = gap + (i / cols) * (ch + gap);
        const Framebuffer& t = thumbs[i];
        for (int y = 0; y < t.height; y++)
            for (int x = 0; x < t.width; x++)
                sheet.setPixel(ox + x, oy + y, t.pixels[y * t.width + x]);
    }
    return sheet;
}

ExportResult exportAnimation(const std::string& name, const Timeline& tl,
                             int width, int height, Color bg,
                             const RenderFrameFn& renderFrame,
                             const ExportOptions& opt) {
    ExportResult res;
    if (tl.frames < 1 || width < 1 || height < 1 || !ensureDirectory(opt.dir)) return res;

    // Which frames go on the contact sheet: evenly spaced, first and last included.
    int k = std::max(1, std::min(opt.sheetFrames, tl.frames));
    std::vector<int> picks;
    for (int j = 0; j < k; j++)
        picks.push_back(k == 1 ? 0 : (int)std::lround(j * (tl.frames - 1.0) / (k - 1)));

    int thumbW = std::max(1, opt.thumbWidth);
    int thumbH = std::max(1, thumbW * height / width);
    std::vector<Framebuffer> thumbs;
    size_t next = 0;

    for (int i = 0; i < tl.frames; i++) {
        Framebuffer fb(width, height, bg);
        renderFrame(fb, tl.tAt(i));
        if (!fb.saveBMP(frameFileName(opt.dir, name, i + 1))) return res;
        res.framesWritten++;
        if (next < picks.size() && picks[next] == i) {
            thumbs.push_back(downscale(fb, thumbW, thumbH));   // keep only small copies
            next++;
        }
    }

    Framebuffer sheet = makeContactSheet(thumbs, opt.sheetColumns, opt.sheetGap, {235, 235, 235});
    res.contactSheetPath = opt.dir + "/" + name + "_sheet.bmp";
    res.ok = sheet.saveBMP(res.contactSheetPath);
    return res;
}