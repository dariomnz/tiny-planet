#pragma once

#include <GLES3/gl3.h>

#include <cstddef>
#include <vector>

enum class BufferTarget : GLenum { Array = GL_ARRAY_BUFFER };
enum class BufferUsage : GLenum { StaticDraw = GL_STATIC_DRAW, DynamicDraw = GL_DYNAMIC_DRAW };

// RAII over VBO. Move-only.
class Buffer {
   public:
    explicit Buffer(BufferTarget target = BufferTarget::Array);
    ~Buffer();

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;
    Buffer(Buffer &&other) noexcept;
    Buffer &operator=(Buffer &&other) noexcept;

    void bind() const;
    void uploadBytes(const void *data, std::size_t bytes, BufferUsage usage) const;
    [[nodiscard]] GLuint id() const noexcept { return m_id; }

    template <typename T>
    void upload(const std::vector<T> &items, BufferUsage usage) const {
        uploadBytes(items.empty() ? nullptr : static_cast<const void *>(items.data()), items.size() * sizeof(T), usage);
    }

   private:
    GLuint m_id = 0;
    GLenum m_target = GL_ARRAY_BUFFER;
};
