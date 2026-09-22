#pragma once

#include <GLES3/gl3.h>

#include <glm/glm.hpp>

#include "gl/Buffer.h"
#include "gl/Program.h"
#include "gl/VertexArray.h"

// Planet grid with curvature + barycentric edges.
class PlanetRenderer {
   public:
    PlanetRenderer();
    void draw(const glm::mat4 &mvp, const glm::vec2 &playerPos, const glm::vec2 &gridOffset, float curveK, float fill,
              const glm::vec3 &edgeColor, float fogDensity) const;

   private:
    Program m_prog;
    VertexArray m_vao;
    Buffer m_vbo;
    GLsizei m_count = 0;
};
