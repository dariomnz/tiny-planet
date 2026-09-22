#pragma once

#include <GLES3/gl3.h>

#include <cstddef>

// RAII over VAO. Move-only.
class VertexArray {
   public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray &) = delete;
    VertexArray &operator=(const VertexArray &) = delete;
    VertexArray(VertexArray &&other) noexcept;
    VertexArray &operator=(VertexArray &&other) noexcept;

    void bind() const;
    static void unbind();
    // Declares layout location=index, float component count, stride/offset in bytes.
    void attrib(GLuint index, GLint size, GLsizei stride, std::size_t offset) const;
    [[nodiscard]] GLuint id() const noexcept { return m_id; }

   private:
    GLuint m_id = 0;
};
