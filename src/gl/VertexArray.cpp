#include "gl/VertexArray.h"

#include "gl/IndexBuffer.h"
#include "gl/Layout.h"
#include "gl/VertexBuffer.h"

VertexArray::VertexArray() { glGenVertexArrays(1, &m_id); }

VertexArray::~VertexArray() {
    if (m_id != 0) glDeleteVertexArrays(1, &m_id);
}

VertexArray::VertexArray(VertexArray &&other) noexcept : m_id(other.m_id), m_attribIndex(other.m_attribIndex) {
    other.m_id = 0;
    other.m_attribIndex = 0;
}

VertexArray &VertexArray::operator=(VertexArray &&other) noexcept {
    if (this != &other) {
        if (m_id != 0) glDeleteVertexArrays(1, &m_id);
        m_id = other.m_id;
        m_attribIndex = other.m_attribIndex;
        other.m_id = 0;
        other.m_attribIndex = 0;
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

void VertexArray::addVertexBuffer(const VertexBuffer &vb) {
    glBindVertexArray(m_id);
    vb.bind();
    const BufferLayout &layout = vb.layout();
    for (const auto &e : layout) {
        const uint32_t count = shaderDataTypeCount(e.type);
        if (shaderDataTypeIsInt(e.type)) {
            glEnableVertexAttribArray(m_attribIndex);
            glVertexAttribIPointer(m_attribIndex, static_cast<GLint>(count), GL_INT,
                                   static_cast<GLsizei>(layout.stride()),
                                   reinterpret_cast<const void *>(e.offset));
        } else {
            glEnableVertexAttribArray(m_attribIndex);
            glVertexAttribPointer(m_attribIndex, static_cast<GLint>(count), GL_FLOAT,
                                  e.normalized ? GL_TRUE : GL_FALSE, static_cast<GLsizei>(layout.stride()),
                                  reinterpret_cast<const void *>(e.offset));
        }
        glVertexAttribDivisor(m_attribIndex, e.divisor);
        ++m_attribIndex;
    }
}

void VertexArray::setIndexBuffer(const IndexBuffer &ib) {
    glBindVertexArray(m_id);
    ib.bind();
}
