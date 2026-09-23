#include "gl/UniformBuffer.h"

UniformBuffer::UniformBuffer(uint32_t size, uint32_t binding) : m_binding(binding) {
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_UNIFORM_BUFFER, m_id);
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(size), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_id);
}

UniformBuffer::~UniformBuffer() {
    if (m_id != 0) glDeleteBuffers(1, &m_id);
}

UniformBuffer::UniformBuffer(UniformBuffer &&other) noexcept : m_id(other.m_id), m_binding(other.m_binding) {
    other.m_id = 0;
}

UniformBuffer &UniformBuffer::operator=(UniformBuffer &&other) noexcept {
    if (this != &other) {
        if (m_id != 0) glDeleteBuffers(1, &m_id);
        m_id = other.m_id;
        m_binding = other.m_binding;
        other.m_id = 0;
    }
    return *this;
}

void UniformBuffer::setData(const void *data, uint32_t size, uint32_t offset) const {
    glBindBuffer(GL_UNIFORM_BUFFER, m_id);
    glBufferSubData(GL_UNIFORM_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
}
