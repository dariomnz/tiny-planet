#include "gl/Buffer.h"

Buffer::Buffer(BufferTarget target) : m_target(static_cast<GLenum>(target)) {
  glGenBuffers(1, &m_id);
}

Buffer::~Buffer() {
  if (m_id != 0)
    glDeleteBuffers(1, &m_id);
}

Buffer::Buffer(Buffer &&other) noexcept
    : m_id(other.m_id), m_target(other.m_target) {
  other.m_id = 0;
}

Buffer &Buffer::operator=(Buffer &&other) noexcept {
  if (this != &other) {
    if (m_id != 0)
      glDeleteBuffers(1, &m_id);
    m_id = other.m_id;
    m_target = other.m_target;
    other.m_id = 0;
  }
  return *this;
}

void Buffer::bind() const { glBindBuffer(m_target, m_id); }

void Buffer::uploadBytes(const void *data, std::size_t bytes,
                         BufferUsage usage) const {
  glBindBuffer(m_target, m_id);
  glBufferData(m_target, static_cast<GLsizeiptr>(bytes), data,
               static_cast<GLenum>(usage));
}
