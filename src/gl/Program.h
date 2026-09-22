#pragma once

#include <GLES3/gl3.h>

#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

class Shader;

// Linked program. Move-only, caches uniform locations.
class Program {
   public:
    Program(const Shader &vert, const Shader &frag);
    // Shortcut: compiles + links in a single call.
    Program(std::string_view vertSrc, std::string_view fragSrc);
    ~Program();

    Program(const Program &) = delete;
    Program &operator=(const Program &) = delete;
    Program(Program &&other) noexcept;
    Program &operator=(Program &&other) noexcept;

    void use() const;
    [[nodiscard]] GLuint id() const noexcept { return m_id; }

    [[nodiscard]] GLint uniformLocation(std::string_view name) const;

    void set(std::string_view name, float v) const;
    void set(std::string_view name, const glm::vec2 &v) const;
    void set(std::string_view name, const glm::vec3 &v) const;
    void set(std::string_view name, const glm::mat4 &m) const;

   private:
    GLuint m_id = 0;
    mutable std::unordered_map<std::string, GLint> m_cache;
};
