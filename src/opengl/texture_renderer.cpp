#include "opengl/texture_renderer.h"
#include "opengl/renderer.h"
#include "opengl/shader.h"
#include "textures/font_atlas.h"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>

namespace gl {

static uint32_t g_prog = 0, g_vao = 0, g_vbo = 0;
static float    g_proj[16];

static const char* kVS = R"(#version 330 core
layout(location = 0) in vec2 aPos;              // unit quad corner, (0,0)..(1,1)
uniform mat4 uProjection;
uniform mat4 uModel;                            // our Mat3 converted to 4x4
uniform vec2 uSize;                             // quad size in local units (pixels)
uniform vec4 uUV;                               // u0, v0, u1, v1
out vec2 vUV;
void main() {
    gl_Position = uProjection * uModel * vec4(aPos * uSize, 0.0, 1.0);
    vUV = mix(uUV.xy, uUV.zw, aPos);
}
)";

// Filter maths below is the same as applyFilterMirror() in filter_mirror.cpp,
// and the same as the C++ reference filters in colour/colour.cpp.
static const char* kFS = R"(#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTexture;
uniform vec4  uColour;                          // multiplied in; a = overall alpha
uniform int   uFilter;                          // 0 none 1 gray 2 sepia 3 tint 4 bright 5 contrast 6 invert
uniform float uStrength;                        // 0 = original, 1 = fully filtered
uniform vec3  uTint;
uniform float uAmount;
void main() {
    vec4 texel = texture(uTexture, vUV);
    vec3 c = texel.rgb;
    vec3 f = c;
    if (uFilter == 1) {
        f = vec3(dot(c, vec3(0.299, 0.587, 0.114)));
    } else if (uFilter == 2) {
        f = vec3(dot(c, vec3(0.393, 0.769, 0.189)),
                 dot(c, vec3(0.349, 0.686, 0.168)),
                 dot(c, vec3(0.272, 0.534, 0.131)));
    } else if (uFilter == 3) {
        f = uTint;
    } else if (uFilter == 4) {
        f = c + vec3(uAmount);
    } else if (uFilter == 5) {
        f = 0.5 + (c - 0.5) * uAmount;
    } else if (uFilter == 6) {
        f = 1.0 - c;
    }
    c = clamp(mix(c, f, clamp(uStrength, 0.0, 1.0)), 0.0, 1.0);
    FragColor = vec4(c * uColour.rgb, texel.a * uColour.a);
}
)";

bool initTextureRenderer() {
    g_prog = createShaderProgram(kVS, kFS);
    if (!g_prog) return false;
    const float quad[] = { 0,0,  1,0,  0,1,  1,1 };     // triangle strip
    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    glEnable(GL_BLEND);                                  // straight-alpha source-over
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setTextureProjection(800, 600);
    return true;
}

void setTextureProjection(int w, int h) {
    orthoMatrix(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f, g_proj);   // y-down, like the polygons
}

void shutdownTextureRenderer() {
    if (g_vbo) glDeleteBuffers(1, &g_vbo);
    if (g_vao) glDeleteVertexArrays(1, &g_vao);
    if (g_prog) deleteShaderProgram(g_prog);
    g_vbo = g_vao = g_prog = 0;
}

Texture createTexture(const Framebuffer& image, bool nearest) {
    Texture t;
    t.width = image.width; t.height = image.height;
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    GLint filt = nearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filt);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filt);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Color is {r,g,b,a} bytes, so the vector is already tightly packed RGBA.
    // Row 0 (top of our image) is uploaded first: uv (0,0) = top-left, matching y-down.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t.width, t.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());
    return t;
}

void deleteTexture(Texture& t) {
    if (t.id) glDeleteTextures(1, &t.id);
    t = Texture();
}

void drawTextureRegion(const Texture& t, const Mat3& model, float sizeW, float sizeH,
                       float u0, float v0, float u1, float v1,
                       Color colour, const FilterParams& filter) {
    if (!t.id || !g_prog) return;
    float m[16];
    mat3ToMat4(model, m);
    glUseProgram(g_prog);
    setUniformMat4(g_prog, "uProjection", g_proj);
    setUniformMat4(g_prog, "uModel", m);
    glUniform2f(glGetUniformLocation(g_prog, "uSize"), sizeW, sizeH);
    glUniform4f(glGetUniformLocation(g_prog, "uUV"), u0, v0, u1, v1);
    glUniform4f(glGetUniformLocation(g_prog, "uColour"), colour.r / 255.0f, colour.g / 255.0f, colour.b / 255.0f, colour.a / 255.0f);
    glUniform1i(glGetUniformLocation(g_prog, "uFilter"), (int)filter.kind);
    glUniform1f(glGetUniformLocation(g_prog, "uStrength"), filter.strength);
    glUniform3f(glGetUniformLocation(g_prog, "uTint"), filter.tint.r / 255.0f, filter.tint.g / 255.0f, filter.tint.b / 255.0f);
    glUniform1f(glGetUniformLocation(g_prog, "uAmount"), filter.amount);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glUniform1i(glGetUniformLocation(g_prog, "uTexture"), 0);
    glBindVertexArray(g_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void drawTexture(const Texture& t, const Mat3& model, float alpha, const FilterParams& filter) {
    uint8_t a = (uint8_t)(std::max(0.0f, std::min(1.0f, alpha)) * 255.0f + 0.5f);
    drawTextureRegion(t, model, (float)t.width, (float)t.height, 0, 0, 1, 1, {255, 255, 255, a}, filter);
}

void drawStampText(const Texture& atlas, const std::string& text,
                   float x, float y, float scale, Color colour, const Mat3& parent) {
    float penX = x;
    for (char ch : text) {
        int gi = glyphIndex(ch);
        if (gi > 0) {                                    // 0 is the blank space
            GlyphUV uv = glyphUV(gi);
            Mat3 m = parent * Mat3::translate(penX, y);
            drawTextureRegion(atlas, m, kGlyphW * scale, kCellH * scale, uv.u0, uv.v0, uv.u1, uv.v1, colour);
        }
        penX += kCellW * scale;                          // same advance as drawText: (3+1)*scale
    }
}

} // namespace gl
