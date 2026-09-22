#include "graphics/Meshes.h"

#include <glm/glm.hpp>

namespace meshes {

std::vector<Vertex2D> makePlanetGrid(int cells, float cell) {
  std::vector<Vertex2D> out;
  out.reserve(static_cast<size_t>(cells * cells * 6));
  const float half = cells * cell * 0.5f;
  const glm::vec3 b0(1, 0, 0), b1(0, 1, 0), b2(0, 0, 1);
  for (int j = 0; j < cells; ++j) {
    for (int i = 0; i < cells; ++i) {
      const float x0 = -half + i * cell, x1 = x0 + cell;
      const float y0 = -half + j * cell, y1 = y0 + cell;
      out.push_back({{x0, y0}, b0});
      out.push_back({{x1, y0}, b1});
      out.push_back({{x0, y1}, b2});
      out.push_back({{x1, y0}, b0});
      out.push_back({{x1, y1}, b1});
      out.push_back({{x0, y1}, b2});
    }
  }
  return out;
}

std::vector<VertexPN> makeBox(float x0, float x1, float y0, float y1, float z0,
                              float z1) {
  using glm::vec3;
  std::vector<VertexPN> out;
  out.reserve(36);
  auto tri = [&](vec3 a, vec3 b, vec3 c, vec3 n) {
    out.push_back({a, n});
    out.push_back({b, n});
    out.push_back({c, n});
  };
  // +X / -X
  tri({x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {1, 0, 0});
  tri({x1, y0, z0}, {x1, y1, z1}, {x1, y0, z1}, {1, 0, 0});
  tri({x0, y0, z0}, {x0, y1, z1}, {x0, y1, z0}, {-1, 0, 0});
  tri({x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {-1, 0, 0});
  // +Y / -Y
  tri({x0, y1, z0}, {x1, y1, z0}, {x1, y1, z1}, {0, 1, 0});
  tri({x0, y1, z0}, {x1, y1, z1}, {x0, y1, z1}, {0, 1, 0});
  tri({x0, y0, z0}, {x1, y0, z1}, {x1, y0, z0}, {0, -1, 0});
  tri({x0, y0, z0}, {x0, y0, z1}, {x1, y0, z1}, {0, -1, 0});
  // top / bottom
  tri({x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {0, 0, 1});
  tri({x0, y0, z1}, {x1, y1, z1}, {x0, y1, z1}, {0, 0, 1});
  tri({x0, y0, z0}, {x1, y1, z0}, {x1, y0, z0}, {0, 0, -1});
  tri({x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, {0, 0, -1});
  return out;
}

} // namespace meshes
