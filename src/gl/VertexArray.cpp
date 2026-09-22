#include "gl/VertexArray.h"

VertexArray::VertexArray() { glGenVertexArrays(1, &m_id); }

VertexArray::~VertexArray() {
    if (m_id != 0) glDeleteVertexArrays(1, &m_id);
}

VertexArray::VertexArray(VertexArray &&other) noexcept : m_id(other.m_id) { other.m_id = 0; }

VertexArray &VertexArray::operator=(VertexArray &&other) noexcept {
    if (this != &other) {
        if (m_id != 0) glDeleteVertexArrays(1, &m_id);
        m_id = other.m_id;
        other.m_id = 0;
    }
    return *this;
}

void VertexArray::bind() const { glBindVertexArray(m_id); }

void VertexArray::unbind() { glBindVertexArray(0); }

void VertexArray::attrib(GLuint index, GLint size, GLsizei stride, std::size_t offset) const {
    glBindVertexArray(m_id);
    glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void *>(offset));
    glEnableVertexAttribArray(index);
}
