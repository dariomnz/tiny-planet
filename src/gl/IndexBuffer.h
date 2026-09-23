#pragma once

#include <GLES3/gl3.h>

#include <cstdint>

// Static index buffer (for future indexed triangle meshes).
class IndexBuffer {
   public:
    IndexBuffer(const uint32_t *indices, uint32_t count);
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer &) = delete;
    IndexBuffer &operator=(const IndexBuffer &) = delete;
    IndexBuffer(IndexBuffer &&other) noexcept;
    IndexBuffer &operator=(IndexBuffer &&other) noexcept;

    void bind() const;
    static void unbind();
    [[nodiscard]] uint32_t count() const noexcept { return m_count; }
    [[nodiscard]] GLuint id() const noexcept { return m_id; }

   private:
    GLuint m_id = 0;
    uint32_t m_count = 0;
};
