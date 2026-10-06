#pragma once
#include "core/framebuffer.h"
void drawCircleMidpoint(Framebuffer&, int cx, int cy, int r, Color);
void fillCircle(Framebuffer&, int cx, int cy, int r, Color);
void drawEllipseMidpoint(Framebuffer&, int cx, int cy, int rx, int ry, Color);
void fillEllipse(Framebuffer&, int cx, int cy, int rx, int ry, Color);