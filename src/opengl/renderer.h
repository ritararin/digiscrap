#pragma once
// -----------------------------------------------------------------------
// Renderer interface: orthographic projection, polygon drawing,
// Mat3→4×4 conversion, and framebuffer-as-texture fallback.
// -----------------------------------------------------------------------

#include <cstdint>
#include <vector>
#include "core/mat3.h"
#include "core/framebuffer.h"

namespace gl {

// ---- Mat3 → 4×4 conversion -------------------------------------------
//
// Our Mat3 is row-major:  m[row][col]
//   [m00  m01  m02]       [a  b  tx]
//   [m10  m11  m12]  i.e. [c  d  ty]
//   [m20  m21  m22]       [0  0   1]
//
// OpenGL expects column-major float[16].  We embed into a 4×4 like so:
//
//   [a   c   0   0]
//   [b   d   0   0]
//   [0   0   1   0]
//   [tx  ty  0   1]
//
void mat3ToMat4(const Mat3& src, float dst[16]);

// Build an orthographic projection matrix (column-major float[16]).
// Default: left=0, right=800, bottom=600, top=0  (y-down, pixel coords).
void orthoMatrix(float left, float right, float bottom, float top,
                 float nearVal, float farVal, float dst[16]);

// ---- Immediate-mode polygon renderer ----------------------------------

struct Vertex2D {
    float x, y;
    float r, g, b, a;
};

// Initialise the renderer (creates VAO/VBO, compiles shaders).
// Call once after the GL context is current.
bool initRenderer();

// Update the projection (call on resize and at startup).
void setProjection(int windowW, int windowH);

// Draw a filled convex polygon as a triangle fan.
void drawPolygon(const Polygon& poly, const Mat3& transform, Color fill);

// Draw a polygon outline.
void drawPolyline(const Polygon& poly, const Mat3& transform, Color col);

// Draw a coloured triangle (sanity-check / demo).
void drawTriangle(float x0, float y0, float x1, float y1, float x2, float y2,
                  Color c0, Color c1, Color c2);

// ---- Framebuffer-as-texture fallback ----------------------------------

// Upload a software-rendered Framebuffer to a GL texture and draw it
// as a full-screen quad.  Creates the texture + quad on first call.
void drawFramebufferTexture(const Framebuffer& fb);

// Clean up all GL resources.
void shutdownRenderer();

} // namespace gl

