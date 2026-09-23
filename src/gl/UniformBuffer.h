#pragma once

#include <GLES3/gl3.h>

#include <cstdint>

// Uniform buffer bound to `binding` (std140). Mirrors Maplex
// OpenGLUniformBuffer, GLES3 port: GenBuffers + BufferData + BindBufferBase,
// SubData update (no DSA).
class UniformBuffer {
   public:
    UniformBuffer(uint32_t size, uint32_t binding);
    ~UniformBuffer();

    UniformBuffer(const UniformBuffer &) = delete;
    UniformBuffer &operator=(const UniformBuffer &) = delete;
    UniformBuffer(UniformBuffer &&other) noexcept;
    UniformBuffer &operator=(UniformBuffer &&other) noexcept;

    void setData(const void *data, uint32_t size, uint32_t offset = 0) const;
    [[nodiscard]] GLuint id() const noexcept { return m_id; }

   private:
    GLuint m_id = 0;
    uint32_t m_binding = 0;
};
