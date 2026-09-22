#pragma once

#include <GLES3/gl3.h>

#include <string_view>

// Shader type with explicit conversion to GLenum.
enum class ShaderType : GLenum {
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
};

// RAII over shader GLuint. Move-only, non-copyable.
class Shader {
   public:
    explicit Shader(ShaderType type, std::string_view src);
    ~Shader();

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&other) noexcept;
    Shader &operator=(Shader &&other) noexcept;

    [[nodiscard]] GLuint id() const noexcept { return m_id; }

   private:
    GLuint m_id = 0;
};
