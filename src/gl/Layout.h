#pragma once

#include <GLES3/gl3.h>

#include <cstdint>
#include <string>
#include <vector>

// Maplex-inspired layout description (OpenGLLayout.hpp), trimmed for
// GLES3/WebGL2: float + int attrs with per-attribute divisor for instancing.
enum class ShaderDataType { Float, Float2, Float3, Float4, Int };

inline uint32_t shaderDataTypeSize(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:
            return 4;
        case ShaderDataType::Float2:
            return 4 * 2;
        case ShaderDataType::Float3:
            return 4 * 3;
        case ShaderDataType::Float4:
            return 4 * 4;
        case ShaderDataType::Int:
            return 4;
    }
    return 0;
}

inline uint32_t shaderDataTypeCount(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:
            return 1;
        case ShaderDataType::Float2:
            return 2;
        case ShaderDataType::Float3:
            return 3;
        case ShaderDataType::Float4:
            return 4;
        case ShaderDataType::Int:
            return 1;
    }
    return 0;
}

inline bool shaderDataTypeIsInt(ShaderDataType type) { return type == ShaderDataType::Int; }

struct BufferElement {
    std::string name;
    ShaderDataType type;
    uint32_t size;
    std::size_t offset = 0;
    uint32_t divisor = 0;
    bool normalized = false;

    BufferElement() = default;
    BufferElement(ShaderDataType t, std::string n, uint32_t d = 0, bool norm = false)
        : name(std::move(n)), type(t), size(shaderDataTypeSize(t)), offset(0), divisor(d), normalized(norm) {}
};

class BufferLayout {
   public:
    BufferLayout() = default;
    BufferLayout(std::initializer_list<BufferElement> elements) : m_elements(elements) { compute(); }

    [[nodiscard]] uint32_t stride() const noexcept { return m_stride; }
    [[nodiscard]] const std::vector<BufferElement> &elements() const noexcept { return m_elements; }
    auto begin() { return m_elements.begin(); }
    auto end() { return m_elements.end(); }
    auto begin() const { return m_elements.begin(); }
    auto end() const { return m_elements.end(); }

   private:
    void compute() {
        std::size_t offset = 0;
        m_stride = 0;
        for (auto &e : m_elements) {
            e.offset = offset;
            offset += e.size;
            m_stride += e.size;
        }
    }

    std::vector<BufferElement> m_elements;
    uint32_t m_stride = 0;
};
