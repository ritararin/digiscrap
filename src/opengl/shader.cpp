#include "opengl/shader.h"

#include <iostream>
#include <vector>
#include <glad/glad.h>

namespace gl {

// ---- Internal helpers -------------------------------------------------

static bool checkCompile(uint32_t shader, const char* label) {
    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok) return true;

    int len;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    std::vector<char> log(len + 1, '\0');
    glGetShaderInfoLog(shader, len, nullptr, log.data());

    std::cerr << "===== SHADER COMPILE ERROR (" << label << ") =====\n"
              << log.data()
              << "==================================================\n";
    return false;
}

static bool checkLink(uint32_t program) {
    int ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok) return true;

    int len;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    std::vector<char> log(len + 1, '\0');
    glGetProgramInfoLog(program, len, nullptr, log.data());

    std::cerr << "===== SHADER LINK ERROR =====\n"
              << log.data()
              << "=============================\n";
    return false;
}

// ---- Public API -------------------------------------------------------

uint32_t createShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    // ---- vertex shader ----
    uint32_t vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);
    if (!checkCompile(vs, "VERTEX")) {
        glDeleteShader(vs);
        return 0;
    }

    // ---- fragment shader ----
    uint32_t fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);
    if (!checkCompile(fs, "FRAGMENT")) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }

    // ---- link program ----
    uint32_t prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    // Shaders can be deleted after linking
    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!checkLink(prog)) {
        glDeleteProgram(prog);
        return 0;
    }

    return prog;
}

void deleteShaderProgram(uint32_t program) {
    if (program) glDeleteProgram(program);
}

void setUniformMat4(uint32_t program, const char* name, const float* mat4) {
    int loc = glGetUniformLocation(program, name);
    if (loc >= 0) {
        glUseProgram(program);
        glUniformMatrix4fv(loc, 1, GL_FALSE, mat4);
    }
}

} // namespace gl

