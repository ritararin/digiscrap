#pragma once
#include <string>
#include <vector>
#include "core/framebuffer.h"
#include "animation/timeline.h"

// "output/anim", "stickers", 1  ->  "output/anim/stickers_0001.bmp"   (index is 1-based)
std::string frameFileName(const std::string& dir, const std::string& name, int index);

// Create the folder (and parents) if needed. True if it exists afterwards.
bool ensureDirectory(const std::string& dir);

// Box-filter shrink (also works for enlarging). Used for contact-sheet thumbnails.
Framebuffer downscale(const Framebuffer& src, int newW, int newH);

// Tile same-sized thumbnails into a grid with a gap around and between them.
Framebuffer makeContactSheet(const std::vector<Framebuffer>& thumbs,
                             int columns, int gap, Color bg);

struct ExportOptions {
    std::string dir = "output/anim";
    int sheetFrames  = 6;     // how many frames go on the contact sheet
    int sheetColumns = 3;
    int thumbWidth   = 240;
    int sheetGap     = 8;
};

struct ExportResult {
    bool        ok = false;
    int         framesWritten = 0;
    std::string contactSheetPath;
};

// Render every frame of `tl` with renderFrame(fb, t), save each as a numbered BMP
// and write <dir>/<name>_sheet.bmp showing evenly spaced frames side by side.
ExportResult exportAnimation(const std::string& name, const Timeline& tl,
                             int width, int height, Color bg,
                             const RenderFrameFn& renderFrame,
                             const ExportOptions& opt = ExportOptions());