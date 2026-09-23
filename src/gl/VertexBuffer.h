#pragma once

#include <GLES3/gl3.h>

#include <cstddef>
#include <cstdint>

#include "gl/Layout.h"

// Dynamic-or-static vertex buffer. Dynamic path mirrors Maplex
// OpenGLVertexBuffer: reserve once with nullptr, per-frame SetData via
// glBufferSubData (no realloc).
class VertexBuffer {
   public:
    // Dynamic reserve: allocates `bytes` with nullptr + DYNAMIC_DRAW.
    explicit VertexBuffer(uint32_t bytes);
    // Static upload.
    VertexBuffer(const void *data, uint32_t bytes);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer &) = delete;
    VertexBuffer &operator=(const VertexBuffer &) = delete;
    VertexBuffer(VertexBuffer &&other) noexcept;
    VertexBuffer &operator=(VertexBuffer &&other) noexcept;

    void bind() const;
    static void unbind();
    // Sub-update [0, size). Size must be <= capacity.
    void setData(const void *data, uint32_t size) const;

    void setLayout(const BufferLayout &layout) { m_layout = layout; }
    [[nodiscard]] const BufferLayout &layout() const noexcept { return m_layout; }
    [[nodiscard]] GLuint id() const noexcept { return m_id; }
    [[nodiscard]] uint32_t capacity() const noexcept { return m_capacity; }

   private:
    GLuint m_id = 0;
    uint32_t m_capacity = 0;
    BufferLayout m_layout;
};
