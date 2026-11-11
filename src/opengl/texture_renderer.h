#pragma once
#include <string>
#include <cstdint>
#include "core/mat3.h"
#include "core/framebuffer.h"
#include "textures/filter_mirror.h"

// Option 3: textures, colour filters and blending on the GPU.
// OpenGL only samples and displays. The sizes, Mat3 placement, filter maths
// and alpha values are our own C++; the Mat3 is converted with mat3ToMat4.
namespace gl {

struct Texture {
    uint32_t id = 0;
    int width = 0, height = 0;
};

bool    initTextureRenderer();                    // call after gl::initRenderer()
void    setTextureProjection(int w, int h);       // same y-down ortho as the polygon renderer
void    shutdownTextureRenderer();

// Upload our RGBA Framebuffer. nearest=true for the pixel font atlas.
Texture createTexture(const Framebuffer& image, bool nearest = false);
void    deleteTexture(Texture& t);

// Draws the whole texture as a w x h quad whose local (0,0) is the top-left
// corner, placed by `model`. alpha is the translucency (washi tape, stickers).
void drawTexture(const Texture& t, const Mat3& model, float alpha = 1.0f,
                 const FilterParams& filter = FilterParams());

// General form: a sub-rectangle of the texture (uv) drawn as a sizeW x sizeH
// quad and multiplied by `colour` (used for tinted text from the font atlas).
void drawTextureRegion(const Texture& t, const Mat3& model, float sizeW, float sizeH,
                       float u0, float v0, float u1, float v1,
                       Color colour, const FilterParams& filter = FilterParams());

// Date-stamp text from the bitmap font atlas. (x, y) is the top-left corner.
void drawStampText(const Texture& fontAtlas, const std::string& text,
                   float x, float y, float scale, Color colour, const Mat3& parent = Mat3::identity());

} // namespace gl
