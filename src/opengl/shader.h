#pragma once
// -----------------------------------------------------------------------
// Shader loading with clear compile / link error messages.
// -----------------------------------------------------------------------

#include <string>
#include <cstdint>

namespace gl {

// Load, compile and link a vertex + fragment shader pair.
// Sources are supplied as C strings (embedded or loaded from disk).
// On failure, prints the exact error(s) to stderr and returns 0.
uint32_t createShaderProgram(const char* vertexSrc, const char* fragmentSrc);

// Delete a program previously created by createShaderProgram().
void deleteShaderProgram(uint32_t program);

// Set common uniforms.
void setUniformMat4(uint32_t program, const char* name, const float* mat4);

} // namespace gl

