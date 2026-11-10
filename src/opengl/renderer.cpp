#include "opengl/renderer.h"
#include "opengl/shader.h"

#include <iostream>
#include <cstring>
#include <glad/glad.h>

namespace gl {

// ---- Embedded shader sources ------------------------------------------

static const char* kBasicVert = R"GLSL(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 uProjection;
uniform mat4 uModel;

out vec4 vColor;

void main() {
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)GLSL";

static const char* kBasicFrag = R"GLSL(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    FragColor = vColor;
}
)GLSL";

static const char* kTextureVert = R"GLSL(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;

out vec2 vUV;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)GLSL";

static const char* kTextureFrag = R"GLSL(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTexture;

void main() {
    FragColor = texture(uTexture, vUV);
}
)GLSL";

// ---- Module state -----------------------------------------------------

static uint32_t g_basicProg   = 0;
static uint32_t g_textureProg = 0;

// Dynamic polygon VAO/VBO
static uint32_t g_polyVAO = 0;
static uint32_t g_polyVBO = 0;

// Full-screen quad VAO/VBO for texture fallback
static uint32_t g_quadVAO = 0;
static uint32_t g_quadVBO = 0;
static uint32_t g_fbTex   = 0;    // framebuffer texture handle

static float g_projection[16];
static int   g_windowW = 800, g_windowH = 600;

// ---- Mat3 → 4×4 ------------------------------------------------------

void mat3ToMat4(const Mat3& src, float dst[16]) {
    // Row-major Mat3  →  column-major 4×4 with z=identity
    //
    //   Mat3:                 GL column-major 4×4:
    //   [m00  m01  m02]       col0  col1  col2  col3
    //   [m10  m11  m12]       m00   m01   0     m02
    //   [m20  m21  m22]       m10   m11   0     m12
    //                         0     0     1     0
    //                         0     0     0     1
    //
    // OpenGL stores column-major: dst[0..3] = col0, dst[4..7] = col1, ...
    std::memset(dst, 0, 16 * sizeof(float));
    dst[0]  = (float)src.m[0][0];  // col0.x
    dst[1]  = (float)src.m[1][0];  // col0.y
    dst[4]  = (float)src.m[0][1];  // col1.x
    dst[5]  = (float)src.m[1][1];  // col1.y
    dst[10] = 1.0f;                // col2.z  (z identity)
    dst[12] = (float)src.m[0][2];  // col3.x  (translation)
    dst[13] = (float)src.m[1][2];  // col3.y
    dst[15] = 1.0f;                // col3.w
}

// ---- Orthographic projection ------------------------------------------

void orthoMatrix(float l, float r, float b, float t,
                 float n, float f, float dst[16]) {
    std::memset(dst, 0, 16 * sizeof(float));
    dst[0]  =  2.0f / (r - l);
    dst[5]  =  2.0f / (t - b);
    dst[10] = -2.0f / (f - n);
    dst[12] = -(r + l) / (r - l);
    dst[13] = -(t + b) / (t - b);
    dst[14] = -(f + n) / (f - n);
    dst[15] = 1.0f;
}

// ---- Renderer init / shutdown -----------------------------------------

bool initRenderer() {
    // ---- basic (polygon) shader ----
    g_basicProg = createShaderProgram(kBasicVert, kBasicFrag);
    if (!g_basicProg) {
        std::cerr << "Failed to compile basic shader program\n";
        return false;
    }

    // ---- texture (framebuffer fallback) shader ----
    g_textureProg = createShaderProgram(kTextureVert, kTextureFrag);
    if (!g_textureProg) {
        std::cerr << "Failed to compile texture shader program\n";
        return false;
    }

    // ---- dynamic polygon VAO/VBO ----
    glGenVertexArrays(1, &g_polyVAO);
    glGenBuffers(1, &g_polyVBO);
    glBindVertexArray(g_polyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_polyVBO);
    // position (2 floats)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
                          (void*)offsetof(Vertex2D, x));
    glEnableVertexAttribArray(0);
    // colour (4 floats)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
                          (void*)offsetof(Vertex2D, r));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // ---- full-screen quad for texture fallback ----
    // NDC quad with UV: two triangles covering [-1,1]×[-1,1]
    float quad[] = {
        // pos       uv
        -1, -1,    0, 0,
         1, -1,    1, 0,
         1,  1,    1, 1,
        -1, -1,    0, 0,
         1,  1,    1, 1,
        -1,  1,    0, 1,
    };
    glGenVertexArrays(1, &g_quadVAO);
    glGenBuffers(1, &g_quadVBO);
    glBindVertexArray(g_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // ---- framebuffer texture (created lazily on first upload) ----
    glGenTextures(1, &g_fbTex);

    // Enable blending for semi-transparent stickers
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set the initial projection
    setProjection(g_windowW, g_windowH);

    std::cout << "Renderer initialised (basic + texture shaders)\n";
    return true;
}

void setProjection(int windowW, int windowH) {
    g_windowW = windowW;
    g_windowH = windowH;
    // y-down pixel coordinates (matches the software Framebuffer convention)
    orthoMatrix(0, (float)windowW, (float)windowH, 0, -1, 1, g_projection);
    if (g_basicProg)
        setUniformMat4(g_basicProg, "uProjection", g_projection);
}

// ---- Drawing helpers --------------------------------------------------

static inline float c2f(uint8_t c) { return c / 255.0f; }

void drawPolygon(const Polygon& poly, const Mat3& transform, Color fill) {
    if (poly.size() < 3) return;

    // Build a triangle fan from the polygon
    std::vector<Vertex2D> verts;
    verts.reserve(poly.size());
    for (const auto& v : poly)
        verts.push_back({(float)v.x, (float)v.y,
                         c2f(fill.r), c2f(fill.g), c2f(fill.b), c2f(fill.a)});

    glBindVertexArray(g_polyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_polyVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex2D),
                 verts.data(), GL_DYNAMIC_DRAW);

    float model[16];
    mat3ToMat4(transform, model);

    glUseProgram(g_basicProg);
    setUniformMat4(g_basicProg, "uProjection", g_projection);
    setUniformMat4(g_basicProg, "uModel", model);

    glDrawArrays(GL_TRIANGLE_FAN, 0, (int)verts.size());
    glBindVertexArray(0);
}

void drawPolyline(const Polygon& poly, const Mat3& transform, Color col) {
    if (poly.size() < 2) return;

    // Close the loop: duplicate the first vertex at the end
    std::vector<Vertex2D> verts;
    verts.reserve(poly.size() + 1);
    for (const auto& v : poly)
        verts.push_back({(float)v.x, (float)v.y,
                         c2f(col.r), c2f(col.g), c2f(col.b), c2f(col.a)});
    verts.push_back(verts[0]);   // close the loop

    glBindVertexArray(g_polyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_polyVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex2D),
                 verts.data(), GL_DYNAMIC_DRAW);

    float model[16];
    mat3ToMat4(transform, model);

    glUseProgram(g_basicProg);
    setUniformMat4(g_basicProg, "uProjection", g_projection);
    setUniformMat4(g_basicProg, "uModel", model);

    glDrawArrays(GL_LINE_STRIP, 0, (int)verts.size());
    glBindVertexArray(0);
}

void drawTriangle(float x0, float y0, float x1, float y1, float x2, float y2,
                  Color c0, Color c1, Color c2) {
    Vertex2D verts[3] = {
        {x0, y0, c2f(c0.r), c2f(c0.g), c2f(c0.b), c2f(c0.a)},
        {x1, y1, c2f(c1.r), c2f(c1.g), c2f(c1.b), c2f(c1.a)},
        {x2, y2, c2f(c2.r), c2f(c2.g), c2f(c2.b), c2f(c2.a)},
    };

    glBindVertexArray(g_polyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_polyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

    float identity[16];
    mat3ToMat4(Mat3::identity(), identity);

    glUseProgram(g_basicProg);
    setUniformMat4(g_basicProg, "uProjection", g_projection);
    setUniformMat4(g_basicProg, "uModel", identity);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

// ---- Framebuffer-as-texture fallback ----------------------------------

void drawFramebufferTexture(const Framebuffer& fb) {
    // Upload pixel data (RGBA) to the texture
    glBindTexture(GL_TEXTURE_2D, g_fbTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Our Framebuffer stores RGBA with y=0 at top.  OpenGL textures have
    // y=0 at bottom, so we flip the rows during upload.
    std::vector<Color> flipped(fb.pixels.size());
    for (int y = 0; y < fb.height; y++)
        std::memcpy(&flipped[(fb.height - 1 - y) * fb.width],
                    &fb.pixels[y * fb.width],
                    fb.width * sizeof(Color));

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 fb.width, fb.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, flipped.data());

    // Draw the full-screen quad
    glUseProgram(g_textureProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_fbTex);
    int loc = glGetUniformLocation(g_textureProg, "uTexture");
    if (loc >= 0) glUniform1i(loc, 0);

    glBindVertexArray(g_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ---- Shutdown ---------------------------------------------------------

void shutdownRenderer() {
    deleteShaderProgram(g_basicProg);
    deleteShaderProgram(g_textureProg);
    g_basicProg = g_textureProg = 0;

    if (g_polyVAO) { glDeleteVertexArrays(1, &g_polyVAO); g_polyVAO = 0; }
    if (g_polyVBO) { glDeleteBuffers(1, &g_polyVBO); g_polyVBO = 0; }
    if (g_quadVAO) { glDeleteVertexArrays(1, &g_quadVAO); g_quadVAO = 0; }
    if (g_quadVBO) { glDeleteBuffers(1, &g_quadVBO); g_quadVBO = 0; }
    if (g_fbTex)   { glDeleteTextures(1, &g_fbTex); g_fbTex = 0; }
}

} // namespace gl

