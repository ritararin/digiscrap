#pragma once
#include "core/framebuffer.h"
void drawLineDDA(Framebuffer&, int x0, int y0, int x1, int y1, Color);
void drawLineBresenham(Framebuffer&, int x0, int y0, int x1, int y1, Color);