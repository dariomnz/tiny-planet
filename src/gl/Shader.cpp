#include "gl/Shader.h"

#include <vector>

#include "gl/Types.h"

Shader::Shader(ShaderType type, std::string_view src) {
  m_id = glCreateShader(static_cast<GLenum>(type));
  const char *ptr = src.data();
  const auto len = static_cast<GLint>(src.size());
  glShaderSource(m_id, 1, &ptr, &len);
  glCompileShader(m_id);
  GLint ok = 0;
  glGetShaderiv(m_id, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint logLen = 0;
    glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &logLen);
    std::vector<char> log(static_cast<size_t>(logLen > 1 ? logLen : 512));
    glGetShaderInfoLog(m_id, static_cast<GLsizei>(log.size()), nullptr,
                       log.data());
    glDeleteShader(m_id);
    m_id = 0;
    throw GlError(std::string("Shader compile error: ") + log.data());
  }
}

Shader::~Shader() {
  if (m_id != 0)
    glDeleteShader(m_id);
}

Shader::Shader(Shader &&other) noexcept : m_id(other.m_id) {
  other.m_id = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept {
  if (this != &other) {
    if (m_id != 0)
      glDeleteShader(m_id);
    m_id = other.m_id;
    other.m_id = 0;
  }
  return *this;
}
