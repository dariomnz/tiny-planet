#include "gl/RenderCommand.h"

#include "gl/VertexArray.h"

void RenderCommand::drawArrays(const VertexArray &vao, uint32_t vertexCount) {
    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
}

void RenderCommand::drawIndexed(const VertexArray &vao, uint32_t indexCount) {
    vao.bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
}

void RenderCommand::drawInstanced(const VertexArray &vao, uint32_t vertexCount, uint32_t instanceCount) {
    vao.bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount),
                          static_cast<GLsizei>(instanceCount));
}

void RenderCommand::drawIndexedInstanced(const VertexArray &vao, uint32_t indexCount, uint32_t instanceCount) {
    vao.bind();
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(instanceCount));
}
