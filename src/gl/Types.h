#pragma once

#include <glm/glm.hpp>
#include <stdexcept>
#include <string>

// Error thrown by the gl/ layer (compile, link).
class GlError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

// Planet vertex: xy + barycentrics (5 floats).
struct Vertex2D {
  glm::vec2 pos{0.0f};
  glm::vec3 bary{0.0f};
};
static_assert(sizeof(Vertex2D) == 5 * sizeof(float), "Vertex2D packing");

// Cube vertex (player/nose/projectile): pos + normal (6 floats).
struct VertexPN {
  glm::vec3 pos{0.0f};
  glm::vec3 normal{0.0f};
};
static_assert(sizeof(VertexPN) == 6 * sizeof(float), "VertexPN packing");
