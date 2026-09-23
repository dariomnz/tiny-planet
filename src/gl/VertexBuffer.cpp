#include "gl/VertexBuffer.h"

VertexBuffer::VertexBuffer(uint32_t bytes) : m_capacity(bytes) {
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bytes), nullptr, GL_DYNAMIC_DRAW);
}

VertexBuffer::VertexBuffer(const void *data, uint32_t bytes) : m_capacity(bytes) {
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bytes), data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer() {
    if (m_id != 0) glDeleteBuffers(1, &m_id);
}

VertexBuffer::VertexBuffer(VertexBuffer &&other) noexcept
    : m_id(other.m_id), m_capacity(other.m_capacity), m_layout(std::move(other.m_layout)) {
    other.m_id = 0;
    other.m_capacity = 0;
}

VertexBuffer &VertexBuffer::operator=(VertexBuffer &&other) noexcept {
    if (this != &other) {
        if (m_id != 0) glDeleteBuffers(1, &m_id);
        m_id = other.m_id;
        m_capacity = other.m_capacity;
        m_layout = std::move(other.m_layout);
        other.m_id = 0;
        other.m_capacity = 0;
    }
    return *this;
}

void VertexBuffer::bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_id); }

void VertexBuffer::unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

void VertexBuffer::setData(const void *data, uint32_t size) const {
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(size), data);
}
