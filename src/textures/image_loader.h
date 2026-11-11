#pragma once
#include <string>
#include "core/framebuffer.h"

// Our own image loaders (no stb_image). Both return false and leave `out`
// untouched when the file is missing or not a format we understand.
//   BMP : uncompressed 24-bit or 32-bit, bottom-up or top-down
//   PPM : binary P6, maxval 255
bool loadBMP(const std::string& path, Framebuffer& out);
bool loadPPM(const std::string& path, Framebuffer& out);

// Picks the loader from the file extension (.bmp / .ppm).
bool loadImage(const std::string& path, Framebuffer& out);

// Scales an image to fit (w x h) using bilinear sampling (our own maths),
// so a large photo becomes a sensible page-sized texture.
Framebuffer resizeBilinear(const Framebuffer& src, int w, int h);

// Procedural photo (sky gradient, sun, hills) used when no file is given.
Framebuffer makeDemoPhoto(int w, int h);
