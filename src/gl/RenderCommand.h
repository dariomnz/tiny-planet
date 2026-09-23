#pragma once

#include <GLES3/gl3.h>

#include <cstdint>

class VertexArray;
class IndexBuffer;

// Thin GLES3 draw dispatcher (Maplex OpenGLRendererAPI subset).
class RenderCommand {
   public:
    static void drawArrays(const VertexArray &vao, uint32_t vertexCount);
    static void drawIndexed(const VertexArray &vao, uint32_t indexCount);
    static void drawInstanced(const VertexArray &vao, uint32_t vertexCount, uint32_t instanceCount);
    static void drawIndexedInstanced(const VertexArray &vao, uint32_t indexCount, uint32_t instanceCount);
};
