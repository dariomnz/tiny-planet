#pragma once

#include <GLES3/gl3.h>

#include <cstddef>
#include <glm/glm.hpp>

#include "gl/Buffer.h"
#include "gl/Program.h"
#include "gl/VertexArray.h"

struct Projectile;

// Cubes with per-face shading + same curvature as the planet.
// Shares a single Program for player/nose/projectiles.
class EntityRenderer {
   public:
    EntityRenderer();

    // Sets projView/playerPos/curveK shared by all draws in the frame.
     void begin(const glm::mat4 &projView, const glm::vec2 &playerPos, float curveK) const;
     void drawPlayer(const glm::mat4 &model, const glm::vec3 &color) const;
     void drawNose(const glm::mat4 &model, const glm::vec3 &color) const;
     void drawProjectiles(const Projectile *items, std::size_t count, const glm::vec3 &color) const;
     void drawEnemies(const glm::vec2 *positions, const float *scales, std::size_t count,
                      const glm::vec3 &color) const;

   private:
    Program m_prog;
    VertexArray m_playerVao;
    Buffer m_playerVbo;
    GLsizei m_playerCount = 0;
    VertexArray m_noseVao;
    Buffer m_noseVbo;
    GLsizei m_noseCount = 0;
    VertexArray m_projVao;
    Buffer m_projVbo;
    GLsizei m_projCount = 0;
};
