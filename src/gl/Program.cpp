#include "gl/Program.h"

#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "gl/Shader.h"
#include "gl/Types.h"

Program::Program(const Shader &vert, const Shader &frag) {
    m_id = glCreateProgram();
    glAttachShader(m_id, vert.id());
    glAttachShader(m_id, frag.id());
    glLinkProgram(m_id);
    GLint ok = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(static_cast<size_t>(logLen > 1 ? logLen : 512));
        glGetProgramInfoLog(m_id, static_cast<GLsizei>(log.size()), nullptr, log.data());
        glDeleteProgram(m_id);
        m_id = 0;
        throw GlError(std::string("Program link error: ") + log.data());
    }
}

Program::Program(std::string_view vertSrc, std::string_view fragSrc)
    : Program(Shader(ShaderType::Vertex, vertSrc), Shader(ShaderType::Fragment, fragSrc)) {}

Program::~Program() {
    if (m_id != 0) glDeleteProgram(m_id);
}

Program::Program(Program &&other) noexcept : m_id(other.m_id), m_cache(std::move(other.m_cache)) { other.m_id = 0; }

Program &Program::operator=(Program &&other) noexcept {
    if (this != &other) {
        if (m_id != 0) glDeleteProgram(m_id);
        m_id = other.m_id;
        m_cache = std::move(other.m_cache);
        other.m_id = 0;
    }
    return *this;
}

void Program::use() const { glUseProgram(m_id); }

GLint Program::uniformLocation(std::string_view name) const {
    const std::string key(name);
    if (auto it = m_cache.find(key); it != m_cache.end()) return it->second;
    const GLint loc = glGetUniformLocation(m_id, key.c_str());
    m_cache.emplace(key, loc);
    return loc;
}

void Program::set(std::string_view name, float v) const { glUniform1f(uniformLocation(name), v); }

void Program::set(std::string_view name, const glm::vec2 &v) const { glUniform2f(uniformLocation(name), v.x, v.y); }

void Program::set(std::string_view name, const glm::vec3 &v) const {
    glUniform3f(uniformLocation(name), v.x, v.y, v.z);
}

void Program::set(std::string_view name, const glm::mat4 &m) const {
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(m));
}
