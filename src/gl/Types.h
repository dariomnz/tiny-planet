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

// Shared frame constants, uploaded once per frame via UBO binding 0.
// std140 layout: mat4 (64) + vec2 (8) + float (4) + pad (4) = 80 bytes.
struct FrameData {
    glm::mat4 projView{1.0f};
    glm::vec2 playerPos{0.0f};
    float curveK = 0.0f;
    float pad = 0.0f;
};
static_assert(sizeof(FrameData) == 80, "FrameData std140 packing");

// Merged unique-mesh vertex: arbitrary triangles with per-vertex color.
// Backs TriBatch (player/nose/future meshes). 9 floats.
struct TriVertex {
    glm::vec3 pos{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec3 color{1.0f};
};
static_assert(sizeof(TriVertex) == 9 * sizeof(float), "TriVertex packing");

// Repeated-mesh instance: shared base mesh + per-instance offset/scale/color.
// Backs InstanceBatch (projectiles/enemies/bullets/gems). 7 floats.
struct InstanceData {
    glm::vec3 offset{0.0f};
    float scale = 1.0f;
    glm::vec3 color{1.0f};
};
static_assert(sizeof(InstanceData) == 7 * sizeof(float), "InstanceData packing");
